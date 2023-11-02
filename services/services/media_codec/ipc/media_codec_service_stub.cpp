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

#include "media_codec_service_stub.h"
#include <unistd.h>
#include "ipc_skeleton.h"
#include "avcodec_dfx.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avcodec_server_manager.h"
#include "avcodec_xcollie.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecServiceStub"};

const std::map<uint32_t, std::string> CODEC_FUNC_NAME = {
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::SET_LISTENER_OBJ),
     "MediaCodecServiceStub SetListenerObject"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::INIT), "MediaCodecServiceStub Init"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::CONFIGURE), "MediaCodecServiceStub Configure"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::START), "MediaCodecServiceStub Start"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::PREPARE), "MediaCodecServiceStub Prepare"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::STOP), "MediaCodecServiceStub Stop"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::FLUSH), "MediaCodecServiceStub Flush"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::RESET), "MediaCodecServiceStub Reset"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::RELEASE), "MediaCodecServiceStub Release"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::GET_OUTPUT_FORMAT),
     "MediaCodecServiceStub GetOutputFormat"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::SET_PARAMETER),
     "MediaCodecServiceStub SetParameter"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::GET_INPUT_BUFFER_QUEUE),
     "MediaCodecServiceStub GetInputBufferQueue"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::SET_OUTPUT_BUFFER_QUEUE),
     "MediaCodecServiceStub SetOutputBufferQueue"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::CREATE_INPUT_SURFACE),
     "MediaCodecServiceStub CreateInputSurface"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::SET_OUTPUT_SURFACE),
     "MediaCodecServiceStub SetOutputSurface"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::NOTIFY_EOS),
     "MediaCodecServiceStub NotifyEos"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::SURFACE_MODE_RETURN_DATA),
     "MediaCodecServiceStub VideoReturnSurfaceModeData"},
    {static_cast<uint32_t>(OHOS::MediaAVCodec::CodecServiceInterfaceCode::DESTROY_STUB),
     "MediaCodecServiceStub DestroyStub"},
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
sptr<MediaCodecServiceStub> MediaCodecServiceStub::Create()
{
    sptr<MediaCodecServiceStub> codecStub = new (std::nothrow) MediaCodecServiceStub();
    CHECK_AND_RETURN_RET_LOG(codecStub != nullptr, nullptr, "Codec service stub create failed");

    int32_t ret = codecStub->InitStub();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec stub init failed");
    return codecStub;
}

MediaCodecServiceStub::MediaCodecServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecServiceStub::~MediaCodecServiceStub()
{
    if (codecServer_ != nullptr) {
        (void)InnerRelease();
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MediaCodecServiceStub::InitStub()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_SYNC_TRACE;
    codecServer_ = MediaCodecServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server create failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::DestroyStub()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    codecServer_ = nullptr;

    AVCodecServerManager::GetInstance().DestroyStubObject(AVCodecServerManager::MEDIA_CODEC, AsObject());
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::DumpInfo(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return std::static_pointer_cast<CodecServer>(codecServer_)->DumpInfo(fd);
}

int32_t MediaCodecServiceStub::SetClientInfo(int32_t clientPid, int32_t clientUid)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return std::static_pointer_cast<CodecServer>(codecServer_)->SetClientInfo(clientPid, clientUid);
}

int MediaCodecServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
                                           MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (MediaCodecServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }
    int32_t ret = -1;
    auto itFuncName = CODEC_FUNC_NAME.find(code);
    std::string funcName =
        itFuncName != CODEC_FUNC_NAME.end() ? itFuncName->second : "MediaCodecServiceStub OnRemoteRequest";
    switch (code) {
        case static_cast<uint32_t>(CodecServiceInterfaceCode::QUEUE_INPUT_BUFFER):
            ret = QueueInputBuffer(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::RELEASE_OUTPUT_BUFFER):
            ret = ReleaseOutputBuffer(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::INIT):
            ret = Init(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::CONFIGURE):
            ret = Configure(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::START):
            ret = Start(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::STOP):
            ret = Stop(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::FLUSH):
            ret = Flush(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::RESET):
            ret = Reset(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::RELEASE):
            ret = Release(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::NOTIFY_EOS):
            ret = NotifyEos(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::CREATE_INPUT_SURFACE):
            ret = CreateInputSurface(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::SET_OUTPUT_SURFACE):
            ret = SetOutputSurface(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::GET_OUTPUT_FORMAT):
            ret = GetOutputFormat(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::SET_PARAMETER):
            ret = SetParameter(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::GET_INPUT_FORMAT):
            ret = GetInputFormat(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::DESTROY_STUB):
            ret = DestroyStub(data, reply);
            break;
        case static_cast<uint32_t>(CodecServiceInterfaceCode::SET_LISTENER_OBJ):
            ret = SetListenerObject(data, reply);
            break;
        default:
            AVCODEC_LOGW("No member func supporting, applying default process");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Failed to call member func %{public}s", funcName.c_str());
    return ret;
}

int32_t MediaCodecServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Object is nullptr");

    listener_ = iface_cast<IStandardMediaCodecListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener is nullptr");

    std::shared_ptr<AVCodecMediaCodecCallback> callback = std::make_shared<MediaCodecListenerCallback>(listener_);

    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    (void)codecServer_->SetCallback(callback);
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Init(bool isEncoder, bool isMimeType, const std::string &name)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    int32_t ret = codecServer_->Init(isEncoder, isMimeType, name);
    if (ret != AVCS_ERR_OK) {
        lock.unlock();
        DestroyStub();
    }
    return ret;
}

int32_t MediaCodecServiceStub::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Configure(format);
}

int32_t MediaCodecServiceStub::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    return codecServer_->Start();
}

int32_t MediaCodecServiceStub::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    return codecServer_->Prepare();
}

int32_t MediaCodecServiceStub::Stop()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    int32_t ret = codecServer_->Stop();
    if (ret == AVCS_ERR_OK) {
        (void)OHOS::IPCSkeleton::FlushCommands(listener_->AsObject().GetRefPtr());
    }
    return ret;
}

int32_t MediaCodecServiceStub::Flush()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    int32_t ret = codecServer_->Flush();
    if (ret == AVCS_ERR_OK) {
        (void)OHOS::IPCSkeleton::FlushCommands(listener_->AsObject().GetRefPtr());
    }
    return ret;
}

int32_t MediaCodecServiceStub::Reset()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    int32_t ret = codecServer_->Reset();
    if (ret == AVCS_ERR_OK) {
        (void)OHOS::IPCSkeleton::FlushCommands(listener_->AsObject().GetRefPtr());
    }
    return ret;
}

int32_t MediaCodecServiceStub::Release()
{
    return InnerRelease();
}

int32_t MediaCodecServiceStub::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetOutputFormat(format);
}

int32_t MediaCodecServiceStub::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetParameter(format);
}

sptr<Media::AVBufferQueueProducer> MediaCodecServiceStub::GetInputBufferQueue()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetInputBufferQueue();
}

int32_t MediaCodecServiceStub::SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetOutputBufferQueue(bufferQueue);
}

sptr<OHOS::Surface> MediaCodecServiceStub::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "Codec server is nullptr");
    return codecServer_->CreateInputSurface();
}

int32_t MediaCodecServiceStub::SetOutputSurface(sptr<OHOS::Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetOutputSurface(surface);
}

int32_t MediaCodecServiceStub::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->NotifyEos();
}

int32_t MediaCodecServiceStub::SurfaceModeReturnData(uint64_t index, bool available)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->VideoReturnSurfaceModeData(buffer, available);
}

int32_t MediaCodecServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(DestroyStub());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    sptr<IRemoteObject> object = data.ReadRemoteObject();

    bool ret = reply.WriteInt32(SetListenerObject(object));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Init(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    bool isEncoder = static_cast<bool>(data.ReadInt32());
    bool isMimeType = data.ReadBool();
    std::string name = data.ReadString();

    bool ret = reply.WriteInt32(Init(isEncoder, isMimeType, name));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Configure(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);

    bool ret = reply.WriteInt32(Configure(format));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    SetClientInfo(IPCSkeleton::GetCallingPid(), IPCSkeleton::GetCallingUid());

    bool ret = reply.WriteInt32(Start());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Prepare(MessageParcel & data, MessageParcel & reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Prepare());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return 0;
}

int32_t MediaCodecServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Stop());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Flush(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Flush());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Reset());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Release());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::NotifyEos(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(NotifyEos());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::CreateInputSurface(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;
    sptr<OHOS::Surface> surface = CreateInputSurface();

    reply.WriteInt32(surface == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK);
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        bool ret = reply.WriteRemoteObject(object);
        CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    }
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::SetOutputSurface(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Object is nullptr");

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, AVCS_ERR_NO_MEMORY, "Producer is nullptr");

    sptr<OHOS::Surface> surface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AVCS_ERR_NO_MEMORY, "Surface create failed");

    std::string format = data.ReadString();
    AVCODEC_LOGI("Surface format is %{public}s!", format.c_str());
    const std::string surfaceFormat = "SURFACE_FORMAT";
    (void)surface->SetUserData(surfaceFormat, format);

    bool ret = reply.WriteInt32(SetOutputSurface(surface));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::GetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    (void)data;
    Format format;
    (void)GetOutputFormat(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::SurfaceModeReturnData(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    uint64_t index = data.ReadUint64();
    bool available = data.ReadBool();

    bool ret = reply.WriteInt32(SurfaceModeReturnData(index, available));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::SetParameter(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);

    bool ret = reply.WriteInt32(SetParameter(format));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::GetInputBufferQueue(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::SetOutputBufferQueue(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::GetInputFormat(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    (void)data;
    Format format;
    (void)GetInputFormat(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceStub::InnerRelease()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Release();
}
} // namespace MediaAVCodec
} // namespace OHOS
