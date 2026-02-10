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

#include "codec_service_stub.h"
#include <unistd.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avcodec_server_manager.h"
#include "avcodec_trace.h"
#include "avcodec_xcollie.h"
#include "codec_listener_proxy.h"
#include "codec_server.h"
#ifdef SUPPORT_DRM
#include "media_key_session_service_proxy.h"
#endif
#include "event_manager.h"
#include "ipc_skeleton.h"
#include "background_event_handler.h"
#include "qos.h"
#include "surface_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecServiceStub"};
constexpr int32_t UID_MEDIA_SERVICE = 1013;
using namespace OHOS::MediaAVCodec;
const std::map<CodecServiceInterfaceCode, std::string> CODEC_FUNC_NAME = {
    {CodecServiceInterfaceCode::QUEUE_INPUT_BUFFER, "QueueInputBuffer"},
    {CodecServiceInterfaceCode::RELEASE_OUTPUT_BUFFER, "ReleaseOutputBuffer"},
    {CodecServiceInterfaceCode::RENDER_OUTPUT_BUFFER_AT_TIME, "RenderOutputBufferAtTime"},
    {CodecServiceInterfaceCode::INIT, "Init"},
    {CodecServiceInterfaceCode::CONFIGURE, "Configure"},
    {CodecServiceInterfaceCode::PREPARE, "Prepare"},
    {CodecServiceInterfaceCode::START, "Start"},
    {CodecServiceInterfaceCode::STOP, "Stop"},
    {CodecServiceInterfaceCode::FLUSH, "Flush"},
    {CodecServiceInterfaceCode::RESET, "Reset"},
    {CodecServiceInterfaceCode::RELEASE, "Release"},
    {CodecServiceInterfaceCode::GET_CHANNEL_ID, "GetChannelId"},
    {CodecServiceInterfaceCode::NOTIFY_EOS, "NotifyEos"},
    {CodecServiceInterfaceCode::CREATE_INPUT_SURFACE, "CreateInputSurface"},
    {CodecServiceInterfaceCode::SET_OUTPUT_SURFACE, "SetOutputSurface"},
    {CodecServiceInterfaceCode::SET_LPP_MODE, "SetLowPowerPlayerMode"},
    {CodecServiceInterfaceCode::GET_OUTPUT_FORMAT, "GetOutputFormat"},
    {CodecServiceInterfaceCode::SET_PARAMETER, "SetParameter"},
    {CodecServiceInterfaceCode::GET_INPUT_FORMAT, "GetInputFormat"},
    {CodecServiceInterfaceCode::GET_CODEC_INFO, "GetCodecInfo"},
    {CodecServiceInterfaceCode::DESTROY_STUB, "DestroyStub"},
    {CodecServiceInterfaceCode::SET_LISTENER_OBJ, "SetListenerObject"},
    {CodecServiceInterfaceCode::SET_DECRYPT_CONFIG, "SetDecryptConfig"},
    {CodecServiceInterfaceCode::SET_CUSTOM_BUFFER, "SetCustomBuffer"},
    {CodecServiceInterfaceCode::NOTIFY_MEMORY_EXCHANGE, "NotifyMemoryExchange"},
    {CodecServiceInterfaceCode::NOTIFY_FREEZE, "NotifyFreeze"},
    {CodecServiceInterfaceCode::NOTIFY_ACTIVE, "NotifyActive"},
    {CodecServiceInterfaceCode::NOTIFY_MEMORY_RECYCLE, "NotifyMemoryRecycle"},
    {CodecServiceInterfaceCode::NOTIFY_MEMORY_WRITE_BACK, "NotifyMemoryWriteBack"},
    {CodecServiceInterfaceCode::NOTIFY_SUSPEND, "NotifySuspend"},
    {CodecServiceInterfaceCode::NOTIFY_RESUME, "NotifyResume"},
};

class QosTool {
public:
    explicit QosTool(OHOS::QOS::QosLevel level)
    {
        OHOS::QOS::SetThreadQos(level);
    }
    ~QosTool()
    {
        OHOS::QOS::ResetThreadQos();
    }

private:
    QosTool() {}
};
#define AVCODEC_FUNC_INTERACTIVE_QOS QosTool qosTool(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE)
} // namespace

namespace OHOS {
namespace MediaAVCodec {
sptr<CodecServiceStub> CodecServiceStub::Create(int32_t instanceId)
{
    sptr<CodecServiceStub> codecStub = new (std::nothrow) CodecServiceStub();
    CHECK_AND_RETURN_RET_LOG(codecStub != nullptr, nullptr, "Codec service stub create failed");

    int32_t ret = codecStub->InitStub(instanceId);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec stub init failed");
    return codecStub;
}

CodecServiceStub::CodecServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecServiceStub::~CodecServiceStub()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    (void)InnerRelease();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t CodecServiceStub::InitStub(int32_t instanceId)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    codecServer_ = CodecServer::Create(instanceId);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server create failed");
    instanceId_ = instanceId;
    recFuncs_[CodecServiceInterfaceCode::INIT] = &CodecServiceStub::Init;
    recFuncs_[CodecServiceInterfaceCode::CONFIGURE] = &CodecServiceStub::Configure;
    recFuncs_[CodecServiceInterfaceCode::PREPARE] = &CodecServiceStub::Prepare;
    recFuncs_[CodecServiceInterfaceCode::START] = &CodecServiceStub::Start;
    recFuncs_[CodecServiceInterfaceCode::STOP] = &CodecServiceStub::Stop;
    recFuncs_[CodecServiceInterfaceCode::FLUSH] = &CodecServiceStub::Flush;
    recFuncs_[CodecServiceInterfaceCode::RESET] = &CodecServiceStub::Reset;
    recFuncs_[CodecServiceInterfaceCode::RELEASE] = &CodecServiceStub::Release;
    recFuncs_[CodecServiceInterfaceCode::GET_CHANNEL_ID] = &CodecServiceStub::GetChannelId;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_EOS] = &CodecServiceStub::NotifyEos;
    recFuncs_[CodecServiceInterfaceCode::QUEUE_INPUT_BUFFER] = &CodecServiceStub::QueueInputBuffer;
    recFuncs_[CodecServiceInterfaceCode::RELEASE_OUTPUT_BUFFER] = &CodecServiceStub::ReleaseOutputBuffer;
    recFuncs_[CodecServiceInterfaceCode::RENDER_OUTPUT_BUFFER_AT_TIME] = &CodecServiceStub::RenderOutputBufferAtTime;
    recFuncs_[CodecServiceInterfaceCode::CREATE_INPUT_SURFACE] = &CodecServiceStub::CreateInputSurface;
    recFuncs_[CodecServiceInterfaceCode::SET_OUTPUT_SURFACE] = &CodecServiceStub::SetOutputSurface;
    recFuncs_[CodecServiceInterfaceCode::SET_LPP_MODE] = &CodecServiceStub::SetLowPowerPlayerMode;
    recFuncs_[CodecServiceInterfaceCode::GET_OUTPUT_FORMAT] = &CodecServiceStub::GetOutputFormat;
    recFuncs_[CodecServiceInterfaceCode::SET_PARAMETER] = &CodecServiceStub::SetParameter;
    recFuncs_[CodecServiceInterfaceCode::GET_INPUT_FORMAT] = &CodecServiceStub::GetInputFormat;
    recFuncs_[CodecServiceInterfaceCode::GET_CODEC_INFO] = &CodecServiceStub::GetCodecInfo;
    recFuncs_[CodecServiceInterfaceCode::DESTROY_STUB] = &CodecServiceStub::DestroyStub;
    recFuncs_[CodecServiceInterfaceCode::SET_LISTENER_OBJ] = &CodecServiceStub::SetListenerObject;
    recFuncs_[CodecServiceInterfaceCode::SET_DECRYPT_CONFIG] = &CodecServiceStub::SetDecryptConfig;
    recFuncs_[CodecServiceInterfaceCode::SET_CUSTOM_BUFFER] = &CodecServiceStub::SetCustomBuffer;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_MEMORY_EXCHANGE] = &CodecServiceStub::NotifyMemoryExchange;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_FREEZE] = &CodecServiceStub::NotifyFreeze;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_ACTIVE] = &CodecServiceStub::NotifyActive;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_MEMORY_RECYCLE] = &CodecServiceStub::NotifyMemoryRecycle;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_MEMORY_WRITE_BACK] = &CodecServiceStub::NotifyMemoryWriteBack;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_SUSPEND] = &CodecServiceStub::NotifySuspend;
    recFuncs_[CodecServiceInterfaceCode::NOTIFY_RESUME] = &CodecServiceStub::NotifyResume;
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::DestroyStub()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    if (codecServer_ == nullptr) {
        return AVCS_ERR_OK;
    }

    auto callerInfo = std::static_pointer_cast<CodecServer>(codecServer_)->GetCallerInfo();
    (void)InnerRelease();
    codecServer_ = nullptr;
    if (listener_ != nullptr) {
        static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->ClearListenerCache();
    }
    AVCodecServerManager::GetInstance().DestroyStubObject(AVCodecServerManager::CODEC, AsObject());
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_RELEASE, *callerInfo);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Dump(int32_t fd, [[maybe_unused]] const std::vector<std::u16string> &args)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return std::static_pointer_cast<CodecServer>(codecServer_)->DumpInfo(fd);
}

int CodecServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (CodecServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE_WITH_TAG("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }
    int32_t ret = AV_ERR_OK;
    auto itFuncName = CODEC_FUNC_NAME.find(static_cast<CodecServiceInterfaceCode>(code));
    auto itFunc = recFuncs_.find(static_cast<CodecServiceInterfaceCode>(code));
    std::string funcName = itFuncName != CODEC_FUNC_NAME.end() ? itFuncName->second : "OnRemoteRequest";
    if (itFunc != recFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            ret = (this->*memberFunc)(data, reply);
        } else {
            ret = AVCS_ERR_UNKNOWN;
        }
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Failed to call member func %{public}s",
                                          funcName.c_str());
        return ret;
    }
    AVCODEC_LOGW_WITH_TAG("No member func supporting, applying default process, code:%{public}u", code);
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t CodecServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Object is nullptr");

    listener_ = iface_cast<IStandardCodecListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener is nullptr");

    std::shared_ptr<MediaCodecCallback> callback = std::make_shared<CodecListenerCallback>(listener_);

    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    (void)codecServer_->SetCallback(callback);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Init(AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    int32_t ret = codecServer_->Init(type, isMimeType, name, callerInfo);
    if (ret != AVCS_ERR_OK) {
        lock.unlock();
        DestroyStub();
    }
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "server is nullptr");
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "listener is nullptr");
    const std::string tag = CreateVideoLogTag(callerInfo);
    this->SetTag(tag);
    codecServer_->SetTag(tag);
    static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->SetTag(tag);
    static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->Init();
    AVCODEC_LOGI_WITH_TAG("%{public}s", AVCSErrorToString(static_cast<AVCodecServiceErrCode>(ret)).c_str());
    return ret;
}

int32_t CodecServiceStub::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Configure(format);
}

int32_t CodecServiceStub::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Prepare();
}

int32_t CodecServiceStub::SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetCustomBuffer(buffer);
}

int32_t CodecServiceStub::NotifyMemoryExchange(const bool exchangeFlag)
{
    return (exchangeFlag ? NotifyFreeze() : NotifyActive()), AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyFreeze()
{
    BackGroundEventHandler::GetInstance().NotifyFreeze(instanceId_);
    return AV_ERR_OK;
}

int32_t CodecServiceStub::NotifyActive()
{
    BackGroundEventHandler::GetInstance().NotifyActive(instanceId_);
    return AV_ERR_OK;
}

int32_t CodecServiceStub::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(codecServer_->CheckRunning()), AVCS_ERR_INVALID_STATE,
                                      "In invalid state, running");
    (void)listener_->UpdateGeneration();
    int32_t ret = codecServer_->Start();
    if (ret != AVCS_ERR_OK) {
        (void)listener_->RestoreGeneration();
    }
    return ret;
}

int32_t CodecServiceStub::Stop()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    OnActive();
    int32_t ret = codecServer_->Stop();
    if (ret == AVCS_ERR_OK) {
        (void)OHOS::IPCSkeleton::FlushCommands(listener_->AsObject().GetRefPtr());
        static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->ClearListenerCache();
    }
    return ret;
}

int32_t CodecServiceStub::Flush()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    OnActive();
    int32_t ret = codecServer_->Flush();
    if (ret == AVCS_ERR_OK) {
        (void)OHOS::IPCSkeleton::FlushCommands(listener_->AsObject().GetRefPtr());
    }
    return ret;
}

int32_t CodecServiceStub::Reset()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listener_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener is nullptr");
    OnActive();
    int32_t ret = codecServer_->Reset();
    if (ret == AVCS_ERR_OK) {
        (void)OHOS::IPCSkeleton::FlushCommands(listener_->AsObject().GetRefPtr());
        static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->ClearListenerCache();
    }
    return ret;
}

int32_t CodecServiceStub::Release()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    OnActive();
    return InnerRelease();
}

int32_t CodecServiceStub::GetChannelId(int32_t &channelId)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetChannelId(channelId);
}

int32_t CodecServiceStub::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->NotifyEos();
}

sptr<OHOS::Surface> CodecServiceStub::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, nullptr, "Codec server is nullptr");
    return codecServer_->CreateInputSurface();
}

int32_t CodecServiceStub::SetOutputSurface(sptr<OHOS::Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetOutputSurface(surface);
}

int32_t CodecServiceStub::SetLowPowerPlayerMode(bool isLpp)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetLowPowerPlayerMode(isLpp);
}

int32_t CodecServiceStub::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->QueueInputBuffer(index, info, flag);
}

int32_t CodecServiceStub::QueueInputBuffer(uint32_t index)
{
    (void)index;
    return AVCS_ERR_UNSUPPORT;
}

int32_t CodecServiceStub::QueueInputParameter(uint32_t index)
{
    (void)index;
    return AVCS_ERR_UNSUPPORT;
}

int32_t CodecServiceStub::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetOutputFormat(format);
}

int32_t CodecServiceStub::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->ReleaseOutputBuffer(index, render);
}

int32_t CodecServiceStub::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->RenderOutputBufferAtTime(index, renderTimestampNs);
}

int32_t CodecServiceStub::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetParameter(format);
}

int32_t CodecServiceStub::GetInputFormat(Format &format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetInputFormat(format);
}

int32_t CodecServiceStub::GetCodecInfo(Format &format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetCodecInfo(format);
}

#ifdef SUPPORT_DRM
int32_t CodecServiceStub::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
                                           const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetDecryptConfig(keySession, svpFlag);
}
#endif

int32_t CodecServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(DestroyStub());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    sptr<IRemoteObject> object = data.ReadRemoteObject();

    bool ret = reply.WriteInt32(SetListenerObject(object));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Init(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_INTERACTIVE_QOS;
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    Meta callerInfo;
    int32_t instanceId = 0;
    std::string codecName = "";
    callerInfo.FromParcel(data);
    AVCodecType type = static_cast<AVCodecType>(data.ReadInt32());
    bool isMimeType = data.ReadBool();
    std::string name = data.ReadString();
    bool parcelRet = reply.WriteInt32(Init(type, isMimeType, name, callerInfo)) &&
                     callerInfo.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId) &&
                     callerInfo.GetData(Tag::MEDIA_CODEC_NAME, codecName);
    Meta replyCallerInfo;
    replyCallerInfo.SetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
    replyCallerInfo.SetData(Tag::MEDIA_CODEC_NAME, codecName);
    parcelRet = parcelRet && replyCallerInfo.ToParcel(reply);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(parcelRet, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Configure(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_INTERACTIVE_QOS;
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);
    bool ret = reply.WriteInt32(Configure(format));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(Prepare());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_INTERACTIVE_QOS;
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(Start());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_INTERACTIVE_QOS;
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(Stop());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Flush(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_INTERACTIVE_QOS;
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(Flush());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_INTERACTIVE_QOS;
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(Reset());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(Release());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::GetChannelId(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    (void)data;
    int32_t channelId;
    (void)GetChannelId(channelId);
    reply.WriteInt32(channelId);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyEos(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;

    bool ret = reply.WriteInt32(NotifyEos());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::CreateInputSurface(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    (void)data;
    sptr<OHOS::Surface> surface = CreateInputSurface();

    reply.WriteInt32(surface == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK);
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        bool ret = reply.WriteRemoteObject(object);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    }
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::SetOutputSurface(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;

    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(object != nullptr, AVCS_ERR_NO_MEMORY, "Object is nullptr");

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(producer != nullptr, AVCS_ERR_NO_MEMORY, "Producer is nullptr");

    sptr<OHOS::Surface> surface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(surface != nullptr, AVCS_ERR_NO_MEMORY, "Surface create failed");

    sptr<OHOS::Surface> surfaceTemp = SurfaceUtils::GetInstance()->GetSurface(surface->GetUniqueId());
    surface = (surfaceTemp != nullptr) ? surfaceTemp : surface;

    std::string format = data.ReadString();
    AVCODEC_LOGD_WITH_TAG("Surface format is %{public}s!", format.c_str());
    const std::string surfaceFormat = "SURFACE_FORMAT";
    (void)surface->SetUserData(surfaceFormat, format);

    bool ret = reply.WriteInt32(SetOutputSurface(surface));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::SetLowPowerPlayerMode(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    bool isLpp = data.ReadBool();
    AVCODEC_LOGD("lpp mode is %{public}d", isLpp);
    bool ret = reply.WriteInt32(SetLowPowerPlayerMode(isLpp));
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::QueueInputBuffer(MessageParcel &data, MessageParcel &reply)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listener_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec listener is nullptr");
    uint32_t index = data.ReadUint32();
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    bool ret =
        static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->InputBufferInfoFromParcel(index, info, flag, data);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Listener read meta data failed");
    AVCODEC_SYNC_CUSTOM_TRACE_WITH_TAG(HITRACE_LEVEL_INFO, "", "%s:S, flag: %u", __FUNCTION__, flag);

    ret = reply.WriteInt32(QueueInputBuffer(index, info, flag));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::GetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;

    (void)data;
    Format format;
    (void)GetOutputFormat(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::ReleaseOutputBuffer(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;

    uint32_t index = data.ReadUint32();
    bool render = data.ReadBool();

    bool ret = reply.WriteInt32(ReleaseOutputBuffer(index, render));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::RenderOutputBufferAtTime(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;

    std::shared_lock<std::shared_mutex> lock(mutex_);
    uint32_t index = data.ReadUint32();
    int64_t renderTimestampNs = data.ReadInt64();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listener_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Codec listener is nullptr");
    static_cast<CodecListenerProxy *>(listener_.GetRefPtr())->SetOutputBufferRenderTimestamp(index, renderTimestampNs);

    bool ret = reply.WriteInt32(RenderOutputBufferAtTime(index, renderTimestampNs));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::SetParameter(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);

    bool ret = reply.WriteInt32(SetParameter(format));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::GetInputFormat(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;

    (void)data;
    Format format;
    (void)GetInputFormat(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::GetCodecInfo(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;

    (void)data;
    Format format;
    (void)GetCodecInfo(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}

#ifdef SUPPORT_DRM
int32_t CodecServiceStub::SetDecryptConfig(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(object != nullptr, AVCS_ERR_INVALID_OPERATION,
                                      "SetDecryptConfig read object is null");
    bool svpFlag = data.ReadBool();

    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy =
        iface_cast<DrmStandard::IMediaKeySessionService>(object);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(keySessionServiceProxy != nullptr, AVCS_ERR_INVALID_OPERATION,
                                      "SetDecryptConfig cast object to proxy failed");
    bool ret = reply.WriteInt32(SetDecryptConfig(keySessionServiceProxy, svpFlag));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}
#else
int32_t CodecServiceStub::SetDecryptConfig(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    (void)reply;
    return AVCS_ERR_OK;
}
#endif

int32_t CodecServiceStub::SetCustomBuffer(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer != nullptr, AVCS_ERR_NO_MEMORY, "Create buffer failed");
    bool ret = buffer->ReadFromMessageParcel(data);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Read From MessageParcel failed");
    ret = reply.WriteInt32(SetCustomBuffer(buffer));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyMemoryExchange(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    auto uid = IPCSkeleton::GetCallingUid();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(
        uid == UID_MEDIA_SERVICE, AVCS_ERR_OK, "Permission denied %{public}d, only supported for media_service", uid);

    bool ret = reply.WriteInt32(NotifyMemoryExchange(data.ReadBool()));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::InnerRelease()
{
    if (isServerReleased_) {
        return AVCS_ERR_NO_MEMORY;
    }
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    int32_t ret = codecServer_->Release();
    isServerReleased_ = (ret == AVCS_ERR_OK);
    return ret;
}

int32_t CodecServiceStub::NotifyMemoryRecycle([[maybe_unused]]MessageParcel &data, MessageParcel &reply)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    auto ret = std::static_pointer_cast<CodecServer>(codecServer_)->NotifyMemoryRecycle();
    isMemoryRecycleFlag_ = ret == AVCS_ERR_OK ? true : false;
    reply.WriteInt32(ret);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyMemoryWriteBack([[maybe_unused]]MessageParcel &data, MessageParcel &reply)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    auto ret = std::static_pointer_cast<CodecServer>(codecServer_)->NotifyMemoryWriteBack();
    isMemoryRecycleFlag_ = ret == AVCS_ERR_OK ? false : true;
    reply.WriteInt32(ret);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifySuspend([[maybe_unused]]MessageParcel &data, MessageParcel &reply)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    auto ret = std::static_pointer_cast<CodecServer>(codecServer_)->NotifySuspend();
    suspended_ = ret == AVCS_ERR_OK ? true : false;
    reply.WriteInt32(ret);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyResume([[maybe_unused]]MessageParcel &data, MessageParcel &reply)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    auto ret = std::static_pointer_cast<CodecServer>(codecServer_)->NotifyResume();
    suspended_ = ret == AVCS_ERR_OK ? false : true;
    reply.WriteInt32(ret);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyFreeze([[maybe_unused]] MessageParcel &data, [[maybe_unused]] MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    return NotifyFreeze();
}

int32_t CodecServiceStub::NotifyActive([[maybe_unused]] MessageParcel &data, [[maybe_unused]] MessageParcel &reply)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    return NotifyActive();
}

void CodecServiceStub::OnActive()
{
    CHECK_AND_RETURN_LOG_WITH_TAG(codecServer_ != nullptr, "Codec server is nullptr");
    AVCODEC_FUNC_TRACE_WITH_TAG_SERVER;
    auto needClear = isMemoryRecycleFlag_ || suspended_;
    if (isMemoryRecycleFlag_) {
        auto ret = std::static_pointer_cast<CodecServer>(codecServer_)->NotifyMemoryWriteBack();
        isMemoryRecycleFlag_ = ret == AVCS_ERR_OK ? false : true;
        needClear = ret == AVCS_ERR_OK ? needClear : false;
    }
    if (suspended_) {
        auto ret = std::static_pointer_cast<CodecServer>(codecServer_)->NotifyResume();
        suspended_ = ret = AVCS_ERR_OK ? false : true;
        needClear = ret == AVCS_ERR_OK ? needClear : false;
    }
    if (needClear) {
        BackGroundEventHandler::GetInstance().EraseInstance(instanceId_);
        AVCODEC_LOGI_WITH_TAG("Done");
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
