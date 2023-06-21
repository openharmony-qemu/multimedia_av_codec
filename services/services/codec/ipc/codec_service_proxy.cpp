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
#include "codec_listener_stub.h"
#include "avsharedmemory_ipc.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServiceProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
class CodecServiceProxy::CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    int32_t ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVSharedMemory> &memory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = caches_.find(index);
        CacheFlag flag = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return AVCS_ERR_INVALID_VAL;
            }
            memory = iter->second;
            return AVCS_ERR_OK;
        }

        if (flag == CacheFlag::UPDATE_CACHE) {
            memory = ReadAVSharedMemoryFromParcel(parcel);
            CHECK_AND_RETURN_RET_LOG(memory != nullptr, AVCS_ERR_INVALID_VAL, "Read memory from parcel failed");

            if (iter == caches_.end()) {
                AVCODEC_LOGI("Add cache, index: %{public}u", index);
                caches_.emplace(index, memory);
            } else {
                iter->second = memory;
                AVCODEC_LOGI("Update cache, index: %{public}u", index);
            }
            return AVCS_ERR_OK;
        }

        // invalidate cache flag
        if (iter != caches_.end()) {
            iter->second = nullptr;
            caches_.erase(iter);
        }
        memory = nullptr;
        AVCODEC_LOGE("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag);
        return AVCS_ERR_INVALID_VAL;
    }

private:
    std::mutex mutex_;
    enum CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };

    std::unordered_map<uint32_t, std::shared_ptr<AVSharedMemory>> caches_;
};

CodecServiceProxy::CodecServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardCodecService>(impl)
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
    int32_t ret = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    data.WriteInt32(static_cast<int32_t>(type));
    data.WriteBool(isMimeType);
    data.WriteString(name);
    int32_t ret = Remote()->SendRequest(INIT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

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
    int32_t ret = Remote()->SendRequest(CONFIGURE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(START, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(STOP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Flush()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(FLUSH, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::NotifyEos()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(NOTIFY_EOS, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Reset()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(RESET, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(RELEASE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

sptr<OHOS::Surface> CodecServiceProxy::CreateInputSurface()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, nullptr, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(CREATE_INPUT_SURFACE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr,
        "Send request failed");

    if (reply.ReadInt32() != AVCS_ERR_OK) {
        return nullptr;
    }
    sptr<IRemoteObject> object = reply.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Read surface object failed");

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
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
    AVCODEC_LOGI("Surface format is %{public}s!", format.c_str());

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)data.WriteRemoteObject(object);
    data.WriteString(format);
    int32_t ret = Remote()->SendRequest(SET_OUTPUT_SURFACE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

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
    data.WriteInt64(info.presentationTimeUs);
    data.WriteInt32(info.size);
    data.WriteInt32(info.offset);
    data.WriteInt32(static_cast<int32_t>(flag));
    int32_t ret = Remote()->SendRequest(QUEUE_INPUT_BUFFER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::GetOutputFormat(Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(GET_OUTPUT_FORMAT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

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
    int32_t ret = Remote()->SendRequest(RELEASE_OUTPUT_BUFFER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

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
    int32_t ret = Remote()->SendRequest(SET_PARAMETER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::GetInputFormat(Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(GET_INPUT_FORMAT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

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

    int32_t ret = Remote()->SendRequest(DESTROY_STUB, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
        "Send request failed");

    return reply.ReadInt32();
}
} // namespace MediaAVCodec
} // namespace OHOS
