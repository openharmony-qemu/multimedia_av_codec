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
#include <string>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"
#include "buffer/avsharedmemorybase.h"
#include "meta/meta.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListenerStub"};
constexpr uint8_t LOG_FREQ = 10;
const std::map<OHOS::Media::MemoryType, std::string> MEMORYTYPE_MAP = {
    {OHOS::Media::MemoryType::VIRTUAL_MEMORY, "VIRTUAL_MEMORY"},
    {OHOS::Media::MemoryType::SHARED_MEMORY, "SHARED_MEMORY"},
    {OHOS::Media::MemoryType::SURFACE_MEMORY, "SURFACE_MEMORY"},
    {OHOS::Media::MemoryType::HARDWARE_MEMORY, "HARDWARE_MEMORY"},
    {OHOS::Media::MemoryType::UNKNOWN_MEMORY, "UNKNOWN_MEMORY"}};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
typedef struct BufferElem {
    std::shared_ptr<AVSharedMemory> memory = nullptr;
    std::shared_ptr<AVBuffer> buffer = nullptr;
    std::shared_ptr<Format> format = nullptr;
} BufferElem;

typedef enum : uint8_t {
    ELEM_GET_AVBUFFER,
    ELEM_GET_AVMEMORY,
    ELEM_GET_AVFORMAT,
} UpdateFilter;

class CodecListenerStub::CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    void ReadFromParcel(uint32_t index, MessageParcel &parcel, BufferElem &elem,
                        const UpdateFilter filter = ELEM_GET_AVBUFFER)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return;
            }
            elem = iter->second;
            HitFunction(elem, parcel, filter);
            return;
        }
        if (flag_ == CacheFlag::UPDATE_CACHE) {
            UpdateFunction(elem, parcel, filter);
            if (iter == caches_.end()) {
                AVCODEC_LOGD("Add cache, index: %{public}u", index);
                caches_.emplace(index, elem);
            } else {
                iter->second = elem;
                AVCODEC_LOGD("Update cache, index: %{public}u", index);
            }
            return;
        }
        // invalidate cache flag_
        if (iter != caches_.end()) {
            caches_.erase(iter);
        }
        AVCODEC_LOGD("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return;
    }

    void GetBufferElem(uint32_t index, BufferElem &elem)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        if (iter == caches_.end()) {
            AVCODEC_LOGE("Get cache failed, index: %{public}u", index);
            return;
        }
        elem = iter->second;
    }

    void ClearCaches()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        caches_.clear();
    }

    void SetIsOutput(bool isOutput)
    {
        isOutput_ = isOutput;
    }

    void SetConverter(std::shared_ptr<BufferConverter> &converter)
    {
        converter_ = converter;
    }

    const std::string GetMemoryTypeStr(const std::shared_ptr<AVBuffer> &buffer)
    {
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, "UNKNOWN_MEMORY", "Invalid buffer");
        if (buffer->memory_ == nullptr) {
            return "UNKNOWN_MEMORY";
        }
        auto iter = MEMORYTYPE_MAP.find(buffer->memory_->GetMemoryType());
        CHECK_AND_RETURN_RET_LOG(iter != MEMORYTYPE_MAP.end(), "UNKNOWN_MEMORY", "unknown memory type");
        return iter->second;
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

    void ReadOutputMemory(std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
    {
        if (converter_ != nullptr) {
            converter_->ReadFromBuffer(buffer, memory);
        }
    }

    void HitFunction(BufferElem &elem, MessageParcel &parcel, const UpdateFilter &filter)
    {
        if (!isOutput_ && (filter == ELEM_GET_AVFORMAT || filter == ELEM_GET_AVBUFFER)) {
            bool isReadSuc = elem.buffer->ReadFromMessageParcel(parcel);
            CHECK_AND_RETURN_LOG(isReadSuc, "Read buffer from parcel failed");
        } else if (isOutput_) {
            bool isReadSuc = elem.buffer->ReadFromMessageParcel(parcel);
            CHECK_AND_RETURN_LOG(isReadSuc, "Read buffer from parcel failed");
            if (filter == ELEM_GET_AVMEMORY) {
                ReadOutputMemory(elem.buffer, elem.memory);
            }
        }
    }

    void UpdateFunction(BufferElem &elem, MessageParcel &parcel, const UpdateFilter &filter)
    {
        elem.buffer = AVBuffer::CreateAVBuffer();
        bool isReadSuc = (elem.buffer != nullptr) && elem.buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG(isReadSuc, "Create buffer from parcel failed");
        if (!isOutput_ && filter == ELEM_GET_AVFORMAT) {
            elem.format = std::make_shared<Format>();
            elem.format->SetMeta(std::move(elem.buffer->meta_));
            elem.buffer->meta_ = elem.format->GetMeta();
        } else if (filter == ELEM_GET_AVMEMORY) {
            AVBufferToAVSharedMemory(elem.buffer, elem.memory);
            if (isOutput_) {
                converter_->SetOutputBufferFormat(elem.buffer);
                ReadOutputMemory(elem.buffer, elem.memory);
            } else {
                converter_->SetInputBufferFormat(elem.buffer);
            }
        }
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
};

CodecListenerStub::CodecListenerStub()
{
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<CodecBufferCache>();
    }

    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<CodecBufferCache>();
        outputBufferCache_->SetIsOutput(true);
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
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (vCb != nullptr) {
        vCb->OnError(errorType, errorCode);
    } else if (cb != nullptr) {
        cb->OnError(errorType, errorCode);
    }
}

void CodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (vCb != nullptr) {
        vCb->OnOutputFormatChanged(format);
    } else if (cb != nullptr) {
        cb->OnOutputFormatChanged(format);
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
    std::shared_ptr<MediaCodecParameterCallback> pCb = paramCallback_.lock();
    if (pCb != nullptr) {
        inputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVFORMAT);
        pCb->OnInputParameterAvailable(index, elem.format);
        elem.buffer->meta_ = elem.format->GetMeta();
        return;
    }
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (vCb != nullptr) {
        inputBufferCache_->ReadFromParcel(index, data, elem);
        vCb->OnInputBufferAvailable(index, elem.buffer);
    } else if (cb != nullptr) {
        inputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVMEMORY);
        cb->OnInputBufferAvailable(index, elem.memory);
    }
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, MessageParcel &data)
{
    BufferElem elem;
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (vCb != nullptr) {
        outputBufferCache_->ReadFromParcel(index, data, elem);
        vCb->OnOutputBufferAvailable(index, elem.buffer);
    } else if (cb != nullptr) {
        outputBufferCache_->ReadFromParcel(index, data, elem, ELEM_GET_AVMEMORY);
        std::shared_ptr<AVBuffer> &buffer = elem.buffer;

        AVCodecBufferInfo info;
        info.presentationTimeUs = buffer->pts_;
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(buffer->flag_);
        if (buffer->memory_ != nullptr) {
            info.offset = buffer->memory_->GetOffset();
            info.size = buffer->memory_->GetSize();
        }
        cb->OnOutputBufferAvailable(index, info, flag, elem.memory);
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

void CodecListenerStub::ClearListenerCache()
{
    inputBufferCache_->ClearCaches();
    outputBufferCache_->ClearCaches();
}

bool CodecListenerStub::WriteInputMemoryToParcel(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                 MessageParcel &data)
{
    BufferElem elem;
    inputBufferCache_->GetBufferElem(index, elem);
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
    inputBufferCache_->GetBufferElem(index, elem);
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
    inputBufferCache_->GetBufferElem(index, elem);
    CHECK_AND_RETURN_RET_LOG(elem.format != nullptr, false, "Get format is nullptr");

    return elem.format->GetMeta()->ToParcel(data);
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
} // namespace MediaAVCodec
} // namespace OHOS
