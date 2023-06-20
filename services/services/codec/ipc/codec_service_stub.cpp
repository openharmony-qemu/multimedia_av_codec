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
#include "avsharedmemory_ipc.h"
#include "codec_listener_proxy.h"
#include "avcodec_xcollie.h"
#include "avcodec_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServiceStub"};

    const std::map<int32_t, std::string> CODEC_FUNC_NAME = {
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::SET_LISTENER_OBJ, "CodecServiceStub SetListenerObject" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::INIT, "CodecServiceStub Init" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::CONFIGURE, "CodecServiceStub Configure" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::START, "CodecServiceStub Start" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::STOP, "CodecServiceStub Stop" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::FLUSH, "CodecServiceStub Flush" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::RESET, "CodecServiceStub Reset" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::RELEASE, "CodecServiceStub Release" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::NOTIFY_EOS, "CodecServiceStub NotifyEos" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::CREATE_INPUT_SURFACE,
            "CodecServiceStub CreateInputSurface" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::SET_OUTPUT_SURFACE, "CodecServiceStub SetOutputSurface" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::GET_INPUT_BUFFER, "CodecServiceStub GetInputBuffer" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::QUEUE_INPUT_BUFFER, "CodecServiceStub QueueInputBuffer" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::GET_OUTPUT_BUFFER, "CodecServiceStub GetOutputBuffer" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::GET_OUTPUT_FORMAT, "CodecServiceStub GetOutputFormat" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::RELEASE_OUTPUT_BUFFER,
            "CodecServiceStub ReleaseOutputBuffer" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::SET_PARAMETER, "CodecServiceStub SetParameter" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::SET_INPUT_SURFACE, "CodecServiceStub SetInputSurface" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::DEQUEUE_INPUT_BUFFER,
            "CodecServiceStub DequeueInputBuffer" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::DEQUEUE_OUTPUT_BUFFER,
            "CodecServiceStub DequeueOutputBuffer" },
        { OHOS::Media::CodecServiceStub::CodecServiceMsg::DESTROY_STUB, "CodecServiceStub DestroyStub" },
    };
}

namespace OHOS {
namespace Media {
class CodecServiceStub::CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    int32_t WriteToParcel(uint32_t index, const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CacheFlag flag = CacheFlag::UPDATE_CACHE;
        if (memory == nullptr || memory->GetBase() == nullptr) {
            AVCODEC_LOGE("Invalid memory for index: %{public}u", index);
            flag = CacheFlag::INVALIDATE_CACHE;
            parcel.WriteUint8(flag);
            auto iter = caches_.find(index);
            if (iter != caches_.end()) {
                iter->second = nullptr;
                caches_.erase(iter);
            }
            return AVCS_ERR_OK;
        }

        auto iter = caches_.find(index);
        if (iter != caches_.end() && iter->second == memory.get()) {
            flag = CacheFlag::HIT_CACHE;
            parcel.WriteUint8(flag);
            return AVCS_ERR_OK;
        }

        if (iter == caches_.end()) {
            AVCODEC_LOGI("Add cached codec buffer, index: %{public}u", index);
            caches_.emplace(index, memory.get());
        } else {
            AVCODEC_LOGI("Update cached codec buffer, index: %{public}u", index);
            iter->second = memory.get();
        }

        parcel.WriteUint8(flag);
        
        return WriteAVSharedMemoryToParcel(memory, parcel);
    }

private:
    std::mutex mutex_;
    enum CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };

    std::unordered_map<uint32_t, AVSharedMemory *> caches_;
};

sptr<CodecServiceStub> CodecServiceStub::Create()
{
    sptr<CodecServiceStub> codecStub = new(std::nothrow) CodecServiceStub();
    CHECK_AND_RETURN_RET_LOG(codecStub != nullptr, nullptr, "Codec service stub create failed");

    int32_t ret = codecStub->InitStub();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec stub init failed");
    return codecStub;
}

CodecServiceStub::CodecServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecServiceStub::~CodecServiceStub()
{
    if (codecServer_ != nullptr) {
        Release();
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t CodecServiceStub::InitStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    AVCODEC_SYNC_TRACE;
    codecServer_ = CodecServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server create failed");

    recFuncs_[SET_LISTENER_OBJ] = &CodecServiceStub::SetListenerObject;
    recFuncs_[INIT] = &CodecServiceStub::Init;
    recFuncs_[CONFIGURE] = &CodecServiceStub::Configure;
    recFuncs_[START] = &CodecServiceStub::Start;
    recFuncs_[STOP] = &CodecServiceStub::Stop;
    recFuncs_[FLUSH] = &CodecServiceStub::Flush;
    recFuncs_[RESET] = &CodecServiceStub::Reset;
    recFuncs_[RELEASE] = &CodecServiceStub::Release;
    recFuncs_[NOTIFY_EOS] = &CodecServiceStub::NotifyEos;
    recFuncs_[CREATE_INPUT_SURFACE] = &CodecServiceStub::CreateInputSurface;
    recFuncs_[SET_OUTPUT_SURFACE] = &CodecServiceStub::SetOutputSurface;
    recFuncs_[GET_INPUT_BUFFER] = &CodecServiceStub::GetInputBuffer;
    recFuncs_[QUEUE_INPUT_BUFFER] = &CodecServiceStub::QueueInputBuffer;
    recFuncs_[GET_OUTPUT_BUFFER] = &CodecServiceStub::GetOutputBuffer;
    recFuncs_[RELEASE_OUTPUT_BUFFER] = &CodecServiceStub::ReleaseOutputBuffer;
    recFuncs_[GET_OUTPUT_FORMAT] = &CodecServiceStub::GetOutputFormat;
    recFuncs_[SET_PARAMETER] = &CodecServiceStub::SetParameter;
    recFuncs_[GET_INPUT_FORMAT] = &CodecServiceStub::GetInputFormat;
    recFuncs_[DESTROY_STUB] = &CodecServiceStub::DestroyStub;
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::DestroyStub()
{
    std::lock_guard<std::mutex> lock(mutex_);
    codecServer_ = nullptr;
    outputBufferCache_ = nullptr;
    inputBufferCache_ = nullptr;

    AVCodecServerManager::GetInstance().DestroyStubObject(AVCodecServerManager::CODEC, AsObject());
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::DumpInfo(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return std::static_pointer_cast<CodecServer>(codecServer_)->DumpInfo(fd);
}

int CodecServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (CodecServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }

    auto itFunc = recFuncs_.find(code);
    if (itFunc != recFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = -1;
            auto itFuncName = CODEC_FUNC_NAME.find(code);
            std::string funcName =
                itFuncName != CODEC_FUNC_NAME.end() ? itFuncName->second : "CodecServiceStub OnRemoteRequest";
            ret = (this->*memberFunc)(data, reply);
            if (ret != AVCS_ERR_OK) {
                AVCODEC_LOGE("Calling member func %{public}s failed.", funcName.c_str());
            }
            return AVCS_ERR_OK;
        }
    }

    AVCODEC_LOGW("No member func supporting, applying default process");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t CodecServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Object is nullptr");

    sptr<IStandardCodecListener> listener = iface_cast<IStandardCodecListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, AVCS_ERR_NO_MEMORY, "Listener is nullptr");

    std::shared_ptr<AVCodecCallback> callback = std::make_shared<CodecListenerCallback>(listener);

    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    (void)codecServer_->SetCallback(callback);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    int32_t ret = codecServer_->Init(type, isMimeType, name);
    if (ret != AVCS_ERR_OK) {
        lock.unlock();
        DestroyStub();
    }
    return ret;
}

int32_t CodecServiceStub::Configure(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<CodecBufferCache>();
    }

    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<CodecBufferCache>();
    }
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Configure(format);
}

int32_t CodecServiceStub::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Start();
}

int32_t CodecServiceStub::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Stop();
}

int32_t CodecServiceStub::Flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->Flush();
}

int32_t CodecServiceStub::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    return codecServer_->Reset();
}

int32_t CodecServiceStub::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    return codecServer_->Release();
}

int32_t CodecServiceStub::NotifyEos()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->NotifyEos();
}

sptr<OHOS::Surface> CodecServiceStub::CreateInputSurface()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "Codec server is nullptr");
    return codecServer_->CreateInputSurface();
}

int32_t CodecServiceStub::SetOutputSurface(sptr<OHOS::Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetOutputSurface(surface);
}

std::shared_ptr<AVSharedMemory> CodecServiceStub::GetInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "Codec server is nullptr");
    return codecServer_->GetInputBuffer(index);
}

int32_t CodecServiceStub::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> CodecServiceStub::GetOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, nullptr, "Codec server is nullptr");
    return codecServer_->GetOutputBuffer(index);
}

int32_t CodecServiceStub::GetOutputFormat(Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetOutputFormat(format);
}

int32_t CodecServiceStub::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->ReleaseOutputBuffer(index, render);
}

int32_t CodecServiceStub::SetParameter(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->SetParameter(format);
}

int32_t CodecServiceStub::GetInputFormat(Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec server is nullptr");
    return codecServer_->GetInputFormat(format);
}

int32_t CodecServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;
    
    bool ret = reply.WriteInt32(DestroyStub());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    sptr<IRemoteObject> object = data.ReadRemoteObject();

    bool ret = reply.WriteInt32(SetListenerObject(object));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Init(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    AVCodecType type = static_cast<AVCodecType>(data.ReadInt32());
    bool isMimeType = data.ReadBool();
    std::string name = data.ReadString();

    bool ret = reply.WriteInt32(Init(type, isMimeType, name));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Configure(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);

    bool ret = reply.WriteInt32(Configure(format));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}


int32_t CodecServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Start());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Stop());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Flush(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Flush());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Reset());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(Release());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::NotifyEos(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    (void)data;

    bool ret = reply.WriteInt32(NotifyEos());
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::CreateInputSurface(MessageParcel &data, MessageParcel &reply)
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

int32_t CodecServiceStub::SetOutputSurface(MessageParcel &data, MessageParcel &reply)
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

int32_t CodecServiceStub::GetInputBuffer(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(inputBufferCache_ != nullptr, AVCS_ERR_NO_MEMORY, "Input buffer cache is nullptr");
    AVCODEC_SYNC_TRACE;

    uint32_t index = data.ReadUint32();
    auto buffer = GetInputBuffer(index);
    reply.WriteInt32(buffer == nullptr ? AVCS_ERR_NO_MEMORY : AVCS_ERR_OK);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_NO_MEMORY, "Buffer is nullptr");

    return inputBufferCache_->WriteToParcel(index, buffer, reply);
}

int32_t CodecServiceStub::QueueInputBuffer(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    uint32_t index = data.ReadUint32();
    AVCodecBufferInfo info;
    info.presentationTimeUs = data.ReadInt64();
    info.size = data.ReadInt32();
    info.offset = data.ReadInt32();
    AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());

    bool ret = reply.WriteInt32(QueueInputBuffer(index, info, flag));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::GetOutputBuffer(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(outputBufferCache_ != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Output buffer cache is nullptr");
    AVCODEC_SYNC_TRACE;

    uint32_t index = data.ReadUint32();
    auto buffer = GetOutputBuffer(index);
    reply.WriteInt32(buffer == nullptr ? AVCS_ERR_NO_MEMORY : AVCS_ERR_OK);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_NO_MEMORY, "Buffer is nullptr");
    
    return outputBufferCache_->WriteToParcel(index, buffer, reply);
}

int32_t CodecServiceStub::GetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    (void)data;
    Format format;
    (void)GetOutputFormat(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::ReleaseOutputBuffer(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    uint32_t index = data.ReadUint32();
    bool render = data.ReadBool();

    bool ret = reply.WriteInt32(ReleaseOutputBuffer(index, render));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::SetParameter(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;
    
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);
    
    bool ret = reply.WriteInt32(SetParameter(format));
    CHECK_AND_RETURN_RET_LOG(ret == true, AVCS_ERR_INVALID_OPERATION, "Reply write failed");
    return AVCS_ERR_OK;
}

int32_t CodecServiceStub::GetInputFormat(MessageParcel &data, MessageParcel &reply)
{
    AVCODEC_SYNC_TRACE;

    (void)data;
    Format format;
    (void)GetInputFormat(format);
    (void)AVCodecParcel::Marshalling(reply, format);
    return AVCS_ERR_OK;
}
} // namespace Media
} // namespace OHOS
