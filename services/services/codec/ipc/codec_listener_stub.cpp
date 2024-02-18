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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecListenerStub"};
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
class CodecListenerStub::CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    void ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVBuffer> &buffer)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return;
            }
            buffer = iter->second.buffer_;
            if (isOutput_) {
                bool isRead = buffer->ReadFromMessageParcel(parcel);
                CHECK_AND_RETURN_LOG(isRead, "Read buffer from parcel failed");
            }
            return;
        }

        if (flag_ == CacheFlag::UPDATE_CACHE) {
            buffer = AVBuffer::CreateAVBuffer();
            CHECK_AND_RETURN_LOG(buffer != nullptr, "Read buffer from parcel failed");
            buffer->ReadFromMessageParcel(parcel);

            if (iter == caches_.end()) {
                AVCODEC_LOGD("Add cache, index: %{public}u, type: %{public}s", index, GetMemoryTypeStr(buffer).c_str());
                BufferAndMemory bufferElem = {.buffer_ = buffer};
                caches_.emplace(index, bufferElem);
            } else {
                iter->second.buffer_ = buffer;
                AVCODEC_LOGD("Update cache, index: %{public}u, type: %{public}s", index,
                             GetMemoryTypeStr(buffer).c_str());
            }
            return;
        }

        // invalidate cache flag_
        if (iter != caches_.end()) {
            caches_.erase(iter);
        }
        buffer = nullptr;
        AVCODEC_LOGD("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return;
    }

    void ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVBuffer> &buffer,
                        std::shared_ptr<AVSharedMemory> &memory)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return;
            }
            buffer = iter->second.buffer_;
            memory = iter->second.memory_;
            if (isOutput_) {
                bool isReadSuc = buffer->ReadFromMessageParcel(parcel);
                CHECK_AND_RETURN_LOG(isReadSuc, "Read buffer from parcel failed");
                ReadOutputMemory(buffer, memory);
            }
            return;
        }
        if (flag_ == CacheFlag::UPDATE_CACHE) {
            buffer = AVBuffer::CreateAVBuffer();
            bool isReadSuc = (buffer != nullptr) && buffer->ReadFromMessageParcel(parcel);
            CHECK_AND_RETURN_LOG(isReadSuc, "Create buffer from parcel failed");
            AVBufferToAVSharedMemory(buffer, memory);
            if (isOutput_) {
                ReadOutputMemory(buffer, memory);
            }
            if (iter == caches_.end()) {
                AVCODEC_LOGD("Add cache, index: %{public}u", index);
                BufferAndMemory bufferElem = {.memory_ = memory, .buffer_ = buffer};
                caches_.emplace(index, bufferElem);
            } else {
                iter->second.buffer_ = buffer;
                iter->second.memory_ = memory;
                AVCODEC_LOGD("Update cache, index: %{public}u", index);
            }
            return;
        }
        // invalidate cache flag_
        if (iter != caches_.end()) {
            caches_.erase(iter);
        }
        buffer = nullptr;
        memory = nullptr;
        AVCODEC_LOGD("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return;
    }

    void GetBufferElem(uint32_t index, std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        if (iter == caches_.end()) {
            buffer = nullptr;
            memory = nullptr;
            AVCODEC_LOGE("Get cache failed, index: %{public}u", index);
            return;
        }
        buffer = iter->second.buffer_;
        memory = iter->second.memory_;
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

    void ReadOutputMemory(const std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
    {
        std::shared_ptr<AVMemory> &bufferMem = buffer->memory_;
        if (bufferMem == nullptr || memory == nullptr || bufferMem->GetMemoryType() == MemoryType::SHARED_MEMORY) {
            return;
        }
        int32_t size = bufferMem->GetSize();
        if (size > 0) {
            int32_t ret = bufferMem->Read(memory->GetBase(), size, 0);
            CHECK_AND_RETURN_LOG(ret == size, "Read avbuffer's data failed.");
        }
    }

    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };
    typedef struct BufferAndMemory {
        std::shared_ptr<AVSharedMemory> memory_ = nullptr;
        std::shared_ptr<AVBuffer> buffer_ = nullptr;
    } BufferAndMemory;
    bool isOutput_ = false;
    CacheFlag flag_ = CacheFlag::INVALIDATE_CACHE;
    std::shared_mutex mutex_;
    std::unordered_map<uint32_t, BufferAndMemory> caches_;
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

    std::unique_lock<std::mutex> lock(syncMutex_, std::try_to_lock);
    CHECK_AND_RETURN_RET_LOG_LIMIT(lock.owns_lock() && CheckGeneration(data.ReadUint64()),
        AVCS_ERR_OK, LOG_FREQ, "abandon message");
    threadId_ = std::this_thread::get_id();
    callbackIsDoing_ = true;
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
            Finalize();
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
    Finalize();
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
    Finalize();
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
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    if (vCb != nullptr) {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        inputBufferCache_->ReadFromParcel(index, data, buffer);
        vCb->OnInputBufferAvailable(index, buffer);
    }

    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        std::shared_ptr<AVSharedMemory> memory = nullptr;
        inputBufferCache_->ReadFromParcel(index, data, buffer, memory);
        cb->OnInputBufferAvailable(index, memory);
    }
    Finalize();
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, MessageParcel &data)
{
    std::shared_ptr<MediaCodecCallback> vCb = videoCallback_.lock();
    if (vCb != nullptr) {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        outputBufferCache_->ReadFromParcel(index, data, buffer);
        vCb->OnOutputBufferAvailable(index, buffer);
    }

    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        std::shared_ptr<AVSharedMemory> memory = nullptr;
        outputBufferCache_->ReadFromParcel(index, data, buffer, memory);

        AVCodecBufferInfo info;
        info.presentationTimeUs = buffer->pts_;
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(buffer->flag_);
        if (buffer->memory_ != nullptr) {
            info.offset = buffer->memory_->GetOffset();
            info.size = buffer->memory_->GetSize();
        }
        cb->OnOutputBufferAvailable(index, info, flag, memory);
    }
    Finalize();
}

void CodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}

void CodecListenerStub::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    videoCallback_ = callback;
}

void CodecListenerStub::WaitCallbackDone()
{
    static std::hash<std::thread::id> hasher;
    if (threadId_ == std::this_thread::get_id()) {
        AVCODEC_LOGI("On the same thread:%{public}" PRIu64 ", so do not wait",
                     static_cast<uint64_t>(hasher(threadId_)));
        return;
    }
    std::unique_lock<std::mutex> lock(syncMutex_);
    syncCv_.wait(lock, [this]() { return !callbackIsDoing_; });
}

void CodecListenerStub::ClearListenerCache()
{
    inputBufferCache_->ClearCaches();
    outputBufferCache_->ClearCaches();
}

bool CodecListenerStub::WriteInputMemoryToParcel(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                 MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    std::shared_ptr<AVSharedMemory> memory = nullptr;
    inputBufferCache_->GetBufferElem(index, buffer, memory);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, false, "Get buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, false, "Get memory is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->memory_ != nullptr, false, "Get buffer memory is nullptr");

    MemoryType type = buffer->memory_->GetMemoryType();
    if (type != MemoryType::SHARED_MEMORY) {
        (void)buffer->memory_->Write(memory->GetBase(), info.size, 0);
    }
    return data.WriteInt64(info.presentationTimeUs) && data.WriteInt32(info.offset) && data.WriteInt32(info.size) &&
           data.WriteUint32(static_cast<uint32_t>(flag)) && buffer->meta_->ToParcel(data);
}

bool CodecListenerStub::WriteInputBufferToParcel(uint32_t index, MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    std::shared_ptr<AVSharedMemory> memory = nullptr;
    inputBufferCache_->GetBufferElem(index, buffer, memory);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, false, "Get buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->memory_ != nullptr, false, "Get buffer memory is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->meta_ != nullptr, false, "Get buffer meta is nullptr");

    return data.WriteInt64(buffer->pts_) && data.WriteInt32(buffer->memory_->GetOffset()) &&
           data.WriteInt32(buffer->memory_->GetSize()) && data.WriteUint32(buffer->flag_) &&
           buffer->meta_->ToParcel(data);
}

bool CodecListenerStub::CheckGeneration(uint64_t messageGeneration) const
{
    return messageGeneration >= GetGeneration();
}

void CodecListenerStub::Finalize()
{
    callbackIsDoing_ = false;
    syncCv_.notify_one();
}
} // namespace MediaAVCodec
} // namespace OHOS
