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

#include "codec_listener_stub.h"
#include <shared_mutex>
#include <sstream>
#include <string>
#include "avcodec_errors.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"
#include "buffer/avsharedmemorybase.h"
#include "meta/meta.h"
namespace {
constexpr uint8_t LOG_FREQ = 10;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
typedef enum : uint8_t {
    OWNED_BY_SERVER = 0,
    OWNED_BY_USER = 1,
} BufferOwner;

typedef struct BufferElem {
    std::shared_ptr<AVSharedMemory> memory = nullptr;
    std::shared_ptr<AVBuffer> buffer = nullptr;
    std::shared_ptr<Format> parameter = nullptr;
    std::shared_ptr<Format> attribute = nullptr;
    BufferOwner owner = OWNED_BY_SERVER;
} BufferElem;

typedef enum : uint8_t {
    ELEM_GET_AVBUFFER,
    ELEM_GET_AVMEMORY,
    ELEM_GET_PARAMETER,
    ELEM_GET_ATRRIBUTE,
} UpdateFilter;

class CodecListenerStub::CodecBufferCache : public NoCopyable {
public:
    explicit CodecBufferCache(bool isOutput) : isOutput_(isOutput) {}
    ~CodecBufferCache() = default;

    bool ReadFromParcel(uint32_t index, MessageParcel &parcel, BufferElem &elem,
                        const UpdateFilter filter = ELEM_GET_AVBUFFER)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE && iter == caches_.end()) {
            AVCODEC_LOGW("Mark hit cache, but can find the index's cache, index: %{public}u", index);
            flag_ = CacheFlag::UPDATE_CACHE;
        }
        if (flag_ == CacheFlag::HIT_CACHE) {
            iter->second.owner = OWNED_BY_USER;
            isOutput_ ? HitOutputCache(iter->second, parcel, filter) : HitInputCache(iter->second, parcel, filter);
            elem = iter->second;
            return CheckReadFromParcelResult(elem, filter);
        }
        if (flag_ == CacheFlag::UPDATE_CACHE) {
            elem.owner = OWNED_BY_USER;
            isOutput_ ? UpdateOutputCache(elem, parcel, filter) : UpdateInputCache(elem, parcel, filter);
            if (iter == caches_.end()) {
                PrintLogOnUpdateBuffer(index);
                caches_.emplace(index, elem);
            } else {
                iter->second = elem;
                PrintLogOnUpdateBuffer(index);
            }
            return CheckReadFromParcelResult(elem, filter);
        }
        // invalidate cache flag_
        if (iter != caches_.end()) {
            caches_.erase(iter);
        }
        AVCODEC_LOGE("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return false;
    }

    void ReturnBufferToServer(uint32_t index, BufferElem &elem)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        if (iter == caches_.end()) {
            AVCODEC_LOGE("Get cache failed, index: %{public}u", index);
            return;
        }
        elem = iter->second;
        if (elem.owner == OWNED_BY_USER) {
            elem.owner = OWNED_BY_SERVER;
        } else {
            AVCODEC_LOGW("Did not receive new callback of this index(%{public}u)", index);
        }
        EXPECT_AND_LOGD(elem.buffer != nullptr, "index=%{public}d, flag=%{public}u, pts=%{public}" PRId64, index,
                        elem.buffer->flag_, elem.buffer->pts_);
    }

    void ClearCaches()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        PrintCachesInfo();
        caches_.clear();
    }

    void FlushCaches()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        PrintCachesInfo();
        for (auto &val : caches_) {
            val.second.owner = OWNED_BY_SERVER;
        }
    }

    void SetConverter(std::shared_ptr<BufferConverter> &converter)
    {
        converter_ = converter;
    }

    inline void PrintLogOnUpdateBuffer(const uint32_t &index)
    {
        if (caches_.size() <= 1) {
            AVCODEC_LOGI("add caches. index: %{public}u", index);
        } else {
            AVCODEC_LOGD("add caches. index: %{public}u", index);
        }
    }

    void PrintCachesInfo()
    {
        std::stringstream serverCaches;
        std::stringstream userCaches;
        serverCaches << "server(";
        userCaches << "user(";
        for (auto &val : caches_) {
            switch (val.second.owner) {
                case OWNED_BY_SERVER:
                    serverCaches << val.first << " ";
                    break;
                case OWNED_BY_USER:
                    userCaches << val.first << " ";
                    break;
                default:
                    break;
            }
        }
        serverCaches << ")";
        userCaches << ")";
        AVCODEC_LOGI("%{public}s caches: %{public}s, %{public}s", (isOutput_ ? "out" : "in"), userCaches.str().c_str(),
                     serverCaches.str().c_str());
    }

    void InitLabel(const uint64_t uid)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        tag_ = isOutput_ ? "OutCache[" : "InCache[";
        tag_ += std::to_string(uid) + "]";
        auto &label = const_cast<OHOS::HiviewDFX::HiLogLabel &>(LABEL);
        label.tag = tag_.c_str();
    }

private:
    void AVBufferToAVSharedMemory(const std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
    {
        using Flags = AVSharedMemory::Flags;
        std::shared_ptr<AVMemory> &bufferMem = buffer->memory_;
        if (bufferMem == nullptr || memory != nullptr) {
            return;
        }
        MemoryType type = bufferMem->GetMemoryType();
        int32_t capacity = bufferMem->GetCapacity();
        if (type == MemoryType::SHARED_MEMORY) {
            std::string name = std::string("SharedMem_") + std::to_string(buffer->GetUniqueId());
            int32_t fd = bufferMem->GetFileDescriptor();
            bool isReadable = bufferMem->GetMemoryFlag() == MemoryFlag::MEMORY_READ_ONLY;
            uint32_t flag = isReadable ? Flags::FLAGS_READ_ONLY : Flags::FLAGS_READ_WRITE;
            memory = AVSharedMemoryBase::CreateFromRemote(fd, capacity, flag, name);
        } else {
            std::string name = std::string("SharedMem_") + std::to_string(buffer->GetUniqueId());
            memory = AVSharedMemoryBase::CreateFromLocal(capacity, Flags::FLAGS_READ_WRITE, name);
            CHECK_AND_RETURN_LOG(memory != nullptr, "Create shared memory from local failed.");
        }
    }

    void HitInputCache(BufferElem &elem, MessageParcel &parcel, const UpdateFilter &filter)
    {
        if (filter == ELEM_GET_AVMEMORY) {
            return;
        }
        bool isReadSuc = elem.buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG(isReadSuc, "Read input buffer from parcel failed");
        elem.buffer->flag_ = 0;
        if (elem.buffer->memory_ != nullptr) {
            elem.buffer->memory_->SetOffset(0);
            elem.buffer->memory_->SetSize(0);
        }
        if (filter == ELEM_GET_ATRRIBUTE) {
            elem.attribute->PutLongValue(Media::Tag::MEDIA_TIME_STAMP, elem.buffer->pts_);
            return;
        }
        if (filter == ELEM_GET_PARAMETER) {
            return;
        }
        elem.buffer->pts_ = 0;
    }

    void HitOutputCache(BufferElem &elem, MessageParcel &parcel, const UpdateFilter &filter)
    {
        bool isReadSuc = elem.buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG(isReadSuc, "Read output buffer from parcel failed");
        if (filter == ELEM_GET_AVMEMORY && converter_ != nullptr) {
            converter_->ReadFromBuffer(elem.buffer, elem.memory);
        }
    }

    void UpdateInputCache(BufferElem &elem, MessageParcel &parcel, const UpdateFilter &filter)
    {
        elem.buffer = AVBuffer::CreateAVBuffer();
        bool isReadSuc = (elem.buffer != nullptr) && elem.buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG(isReadSuc, "Create input buffer from parcel failed");
        if (filter == ELEM_GET_PARAMETER) {
            elem.parameter = std::make_shared<Format>();
            elem.parameter->SetMeta(std::move(elem.buffer->meta_));
            elem.buffer->meta_ = elem.parameter->GetMeta();
        } else if (filter == ELEM_GET_ATRRIBUTE) {
            elem.parameter = std::make_shared<Format>();
            elem.parameter->SetMeta(std::move(elem.buffer->meta_));
            elem.buffer->meta_ = elem.parameter->GetMeta();

            elem.attribute = std::make_shared<Format>();
            elem.attribute->PutLongValue(Media::Tag::MEDIA_TIME_STAMP, elem.buffer->pts_);
        } else if (filter == ELEM_GET_AVMEMORY) {
            AVBufferToAVSharedMemory(elem.buffer, elem.memory);
            if (converter_ != nullptr) {
                converter_->SetInputBufferFormat(elem.buffer);
            }
        }
    }

    void UpdateOutputCache(BufferElem &elem, MessageParcel &parcel, const UpdateFilter &filter)
    {
        elem.buffer = AVBuffer::CreateAVBuffer();
        bool isReadSuc = (elem.buffer != nullptr) && elem.buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG(isReadSuc, "Create output buffer from parcel failed");
        if (filter == ELEM_GET_AVMEMORY) {
            AVBufferToAVSharedMemory(elem.buffer, elem.memory);
            if (converter_ != nullptr) {
                converter_->SetOutputBufferFormat(elem.buffer);
                converter_->ReadFromBuffer(elem.buffer, elem.memory);
            }
        }
    }

    bool CheckReadFromParcelResult(const BufferElem &elem, const UpdateFilter filter)
    {
        switch (filter) {
            case ELEM_GET_AVBUFFER:
                return true;
            case ELEM_GET_AVMEMORY:
                return elem.buffer != nullptr;
            case ELEM_GET_PARAMETER:
                return elem.buffer != nullptr && elem.parameter != nullptr;
            case ELEM_GET_ATRRIBUTE:
                return elem.buffer != nullptr && elem.parameter != nullptr && elem.attribute != nullptr;
            default:
                AVCODEC_LOGE("unknown filter:%{public}d", static_cast<int32_t>(filter));
                break;
        }
        return false;
    }

    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };
    bool isOutput_ = false;
    CacheFlag flag_ = CacheFlag::INVALIDATE_CACHE;
    std::shared_mutex mutex_;
    std::unordered_map<uint32_t, BufferElem> caches_;
    std::shared_ptr<BufferConverter> converter_ = nullptr;

    const OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "ClientCaches"};
    std::string tag_ = "";
};

CodecListenerStub::CodecListenerStub()
{
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<CodecBufferCache>(false);
    }

    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<CodecBufferCache>(true);
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListenerStub::~CodecListenerStub()
{
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int CodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(CodecListenerStub::GetDescriptor() == remoteDescriptor, AVCS_ERR_INVALID_OPERATION,
                             "Invalid descriptor");
    CHECK_AND_RETURN_RET_LOG(inputBufferCache_ != nullptr, AVCS_ERR_INVALID_OPERATION, "inputBufferCache is nullptr");
    CHECK_AND_RETURN_RET_LOG(outputBufferCache_ != nullptr, AVCS_ERR_INVALID_OPERATION, "outputBufferCache is nullptr");

    CHECK_AND_RETURN_RET_LOG(syncMutex_ != nullptr, AVCS_ERR_INVALID_OPERATION, "sync mutex is nullptr");
    std::lock_guard<std::recursive_mutex> lock(*syncMutex_);
    if (!needListen_ || !CheckGeneration(data.ReadUint64())) {
        AVCODEC_LOGW_LIMIT(LOG_FREQ, "abandon message");
        return AVCS_ERR_OK;
    }
    switch (code) {
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_ERROR): {
            int32_t errorType = data.ReadInt32();
            int32_t errorCode = data.ReadInt32();
            OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_FORMAT_CHANGED): {
            Format format;
            (void)AVCodecParcel::Unmarshalling(data, format);
            outputBufferCache_->ClearCaches();
            OnOutputFormatChanged(format);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_INPUT_BUFFER_AVAILABLE): {
            uint32_t index = data.ReadUint32();
            OnInputBufferAvailable(index, data);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_AVAILABLE): {
            uint32_t index = data.ReadUint32();
            OnOutputBufferAvailable(index, data);
            return AVCS_ERR_OK;
        }
        default: {
            AVCODEC_LOGE("Default case, please check codec listener stub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void CodecListenerStub::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    if (vCb != nullptr) {
        vCb->OnError(errorType, errorCode);
        return;
    }
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnError(errorType, errorCode);
        return;
    }
}

void CodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    if (vCb != nullptr) {
        vCb->OnOutputFormatChanged(format);
        return;
    }
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnOutputFormatChanged(format);
        return;
    }
}

void CodecListenerStub::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (void)index;
    (void)buffer;
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (void)index;
    (void)buffer;
}

void CodecListenerStub::OnInputBufferAvailable(uint32_t index, MessageParcel &data)
{
    BufferElem elem;
    std::shared_ptr<MediaCodecParameterCallback> paramCb = paramCallback_.lock();
    if (paramCb != nullptr) {
        bool ret = inputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_PARAMETER);
        CHECK_AND_RETURN_LOG(ret, "read from parel failed");
        paramCb->OnInputParameterAvailable(index, elem.parameter);
        elem.buffer->meta_ = elem.parameter->GetMeta();
        return;
    }
    std::shared_ptr<MediaCodecParameterWithAttrCallback> attrCb = paramWithAttrCallback_.lock();
    if (attrCb != nullptr) {
        bool ret = inputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_ATRRIBUTE);
        CHECK_AND_RETURN_LOG(ret, "read from parel failed");
        attrCb->OnInputParameterWithAttrAvailable(index, elem.attribute, elem.parameter);
        elem.buffer->meta_ = elem.parameter->GetMeta();
        return;
    }
    std::shared_ptr<MediaCodecCallback> mediaCb = videoCallback_.lock();
    if (mediaCb != nullptr) {
        bool ret = inputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVBUFFER);
        CHECK_AND_RETURN_LOG(ret, "read from parel failed");
        mediaCb->OnInputBufferAvailable(index, elem.buffer);
        return;
    }
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        bool ret = inputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVMEMORY);
        CHECK_AND_RETURN_LOG(ret, "read from parel failed");
        cb->OnInputBufferAvailable(index, elem.memory);
        return;
    }
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, MessageParcel &data)
{
    BufferElem elem;
    std::shared_ptr<MediaCodecCallback> mediaCb = videoCallback_.lock();
    if (mediaCb != nullptr) {
        bool ret = outputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVBUFFER);
        CHECK_AND_RETURN_LOG(ret, "read from parel failed");
        mediaCb->OnOutputBufferAvailable(index, elem.buffer);
        return;
    }
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        bool ret = outputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVMEMORY);
        CHECK_AND_RETURN_LOG(ret, "read from parel failed");
        std::shared_ptr<AVBuffer> &buffer = elem.buffer;
        AVCodecBufferInfo info;
        info.presentationTimeUs = buffer->pts_;
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(buffer->flag_);
        if (buffer->memory_ != nullptr) {
            info.offset = buffer->memory_->GetOffset();
            info.size = buffer->memory_->GetSize();
        }
        cb->OnOutputBufferAvailable(index, info, flag, elem.memory);
        return;
    }
}

void CodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}

void CodecListenerStub::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    videoCallback_ = callback;
}

void CodecListenerStub::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    paramCallback_ = callback;
}

void CodecListenerStub::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    paramWithAttrCallback_ = callback;
}

void CodecListenerStub::ClearListenerCache()
{
    inputBufferCache_->ClearCaches();
    outputBufferCache_->ClearCaches();
}

void CodecListenerStub::FlushListenerCache()
{
    inputBufferCache_->FlushCaches();
    outputBufferCache_->FlushCaches();
}

bool CodecListenerStub::WriteInputMemoryToParcel(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                 MessageParcel &data)
{
    BufferElem elem;
    inputBufferCache_->ReturnBufferToServer(index, elem);
    std::shared_ptr<AVBuffer> &buffer = elem.buffer;
    std::shared_ptr<AVSharedMemory> &memory = elem.memory;
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, false, "Get buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, false, "Get memory is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->memory_ != nullptr, false, "Get buffer memory is nullptr");

    if (converter_ != nullptr) {
        buffer->memory_->SetSize(info.size);
        converter_->WriteToBuffer(buffer, memory);
    }
    return data.WriteInt64(info.presentationTimeUs) && data.WriteInt32(info.offset) &&
           data.WriteInt32(buffer->memory_->GetSize()) && data.WriteUint32(static_cast<uint32_t>(flag)) &&
           buffer->meta_->ToParcel(data);
}

bool CodecListenerStub::WriteInputBufferToParcel(uint32_t index, MessageParcel &data)
{
    BufferElem elem;
    inputBufferCache_->ReturnBufferToServer(index, elem);
    std::shared_ptr<AVBuffer> &buffer = elem.buffer;
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, false, "Get buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->memory_ != nullptr, false, "Get buffer memory is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->meta_ != nullptr, false, "Get buffer meta is nullptr");

    return data.WriteInt64(buffer->pts_) && data.WriteInt32(buffer->memory_->GetOffset()) &&
           data.WriteInt32(buffer->memory_->GetSize()) && data.WriteUint32(buffer->flag_) &&
           buffer->meta_->ToParcel(data);
}

bool CodecListenerStub::WriteInputParameterToParcel(uint32_t index, MessageParcel &data)
{
    BufferElem elem;
    inputBufferCache_->ReturnBufferToServer(index, elem);
    auto &param = elem.parameter;
    CHECK_AND_RETURN_RET_LOG(elem.buffer != nullptr, false, "Get buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(param != nullptr, false, "Get format is nullptr");
    EXPECT_AND_LOGD(!(param->GetMeta()->Empty()), "index:%{public}u,pts:%{public}" PRId64 ",paramter:%{public}s", index,
                    elem.buffer->pts_, param->Stringify().c_str());

    return param->GetMeta()->ToParcel(data);
}

bool CodecListenerStub::WriteOutputBufferToParcel(uint32_t index, MessageParcel &data)
{
    (void)data;
    BufferElem elem;
    outputBufferCache_->ReturnBufferToServer(index, elem);
    return true;
}

bool CodecListenerStub::CheckGeneration(uint64_t messageGeneration) const
{
    return messageGeneration >= GetGeneration();
}

void CodecListenerStub::SetMutex(std::shared_ptr<std::recursive_mutex> &mutex)
{
    syncMutex_ = mutex;
}

void CodecListenerStub::SetConverter(std::shared_ptr<BufferConverter> &converter)
{
    converter_ = converter;
    inputBufferCache_->SetConverter(converter);
    outputBufferCache_->SetConverter(converter);
}

void CodecListenerStub::SetNeedListen(const bool needListen)
{
    needListen_ = needListen;
}

void CodecListenerStub::InitLabel(const uint64_t uid)
{
    tag_ = "ListenerStub[";
    tag_ += std::to_string(uid) + "]";
    auto &label = const_cast<OHOS::HiviewDFX::HiLogLabel &>(LABEL);
    label.tag = tag_.c_str();
    inputBufferCache_->InitLabel(uid);
    outputBufferCache_->InitLabel(uid);
}
} // namespace MediaAVCodec
} // namespace OHOS
