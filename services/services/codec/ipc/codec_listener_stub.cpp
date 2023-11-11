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
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecListenerStub"};
}

using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
class CodecListenerStub::CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    void ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVBuffer> &buffer)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return;
            }
            buffer = iter->second.buffer_;
            return;
        }

        if (flag_ == CacheFlag::UPDATE_CACHE) {
            buffer = AVBuffer::CreateAVBuffer(parcel);
            CHECK_AND_RETURN_LOG(buffer != nullptr, "Read buffer from parcel failed");

            if (iter == caches_.end()) {
                AVCODEC_LOGI("Add cache, index: %{public}u", index);
                BufferAndMemory bufferElem = {.buffer_ = buffer};
                caches_.emplace(index, bufferElem);
            } else {
                iter->second.buffer_ = buffer;
                AVCODEC_LOGI("Update cache, index: %{public}u", index);
            }
            return;
        }

        // invalidate cache flag_
        if (iter != caches_.end()) {
            // iter->second = nullptr;
            caches_.erase(iter);
        }
        buffer = nullptr;
        AVCODEC_LOGD("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return;
    }

    void ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVSharedMeory> &memory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::shared_ptr<AVBuffer> buffer = nullptr;
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return;
            }
            buffer = iter->second.buffer_;
            memory = iter->second.memory_;
            AVBufferToAVSharedMemory(buffer, memory);
            return;
        }

        if (flag_ == CacheFlag::UPDATE_CACHE) {
            buffer = AVBuffer::CreateAVBuffer(parcel);
            CHECK_AND_RETURN_LOG(buffer != nullptr, "Read buffer from parcel failed");
            AVBufferToAVSharedMemory(buffer, memory);
            CHECK_AND_RETURN_LOG(memory != nullptr, "Read memory from parcel failed");

            if (iter == caches_.end()) {
                AVCODEC_LOGI("Add cache, index: %{public}u", index);
                BufferAndMemory bufferElem = {.buffer_ = buffer, .memory_ = memory};
                caches_.emplace(index, bufferElem);
            } else {
                iter->second.buffer_ = buffer;
                iter->second.memory_ = memory;
                AVCODEC_LOGI("Update cache, index: %{public}u", index);
            }
            return;
        }

        // invalidate cache flag_
        if (iter != caches_.end()) {
            // iter->second = nullptr;
            caches_.erase(iter);
        }
        buffer = nullptr;
        memory = nullptr;
        AVCODEC_LOGD("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return;
    }

    void GetBufferElem(uint32_t index, std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
    {
        auto iter = caches_.find(index);
        if (iter == cahces_.end()) {
            buffer = nullptr;
            memory = nullptr;
            AVCODEC_LOGI("Get cache failed, index: %{public}u", index);
            return;
        }
        buffer = iter->secnond.buffer_;
        memory = iter->second.memory_;
    }

    void ClearCaches()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        caches_.clear();
    }

private:
    void AVBufferToAVSharedMemory(const std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
    {
        std::shared_ptr<AVMemory> &mem = buffer->memory_;
        CHECK_AND_RETURN_LOG(mem != nullptr, "AVBuffer's memory is nullptr.");
        MemoryType type = mem->GetMemoryType();

        if (flag_ == HIT_CACHE && type == MemoryType::SHARED_MEMORY) {
            return;
        }
        int32_t capacity = mem->GetCapacity();

        std::string name = "sharedMemory_" + std::to_string(index);
        if (type == MemoryType::SHARED_MEMORY) {
            int32_t fd = mem->GetFileDescriptor();
            bool isReadable = mem->GetMemoryFlag() == MemoryFlag::MEMORY_READ_ONLY;
            uint32_t flag = isReadable ? Flags::FLAGS_READ_ONLY : Flags::FLAGS_READ_WRITE;
            memory = AVSharedMemoryBase::CreateFromRemote(fd, capacity, flag, name);
            return;
        } else {
            int32_t size = mem->GetSize();
            uint32_t flag = Flags::FLAGS_READ_WRITE;
            memory = AVSharedMemoryBase::CreateFromLocal(capacity, flag, name);
            CHECK_AND_RETURN_LOG(memory != nullptr, "Create shared memory from local failed.");

            int32_t ret = mem->Read(memory->GetBase(), size, 0);
            CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Read avbuffer's data failed.");
            return;
        }
        return;
    }

    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };
    using BufferAndMemory = struct BufferAndMemory {
        std::shared_ptr<AVSharedMemory> memory_ = nullptr;
        std::shared_ptr<AVBuffer> buffer_ = nullptr;
    };
    // std::unordered_map<uint32_t, std::shared_ptr<AVSharedMemory>> caches_;
    std::mutex mutex_;
    CacheFlag flag_;
    std::unordered_map<uint32_t, BufferAndMemory> caches_;
};

CodecListenerStub::CodecListenerStub()
{
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<CodecBufferCache>();
    }

    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<CodecBufferCache>();
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
    CHECK_AND_RETURN_RET_LOG(lock.owns_lock() && CheckGeneration(data.ReadUint64()), AVCS_ERR_OK, "abandon message");

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
            std::shared_ptr<AVBuffer> buffer = nullptr;
            inputBufferCache_->ReadFromParcel(index, data, buffer);
            OnInputBufferAvailable(index, buffer);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_AVAILABLE): {
            uint32_t index = data.ReadUint32();
            AVCodecBufferInfo info{data.ReadInt64(), data.ReadInt32(), data.ReadInt32()};
            AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());
            std::shared_ptr<AVBuffer> buffer = nullptr;
            outputBufferCache_->ReadFromParcel(index, buffer);
            OnOutputBufferAvailable(index, buffery);
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
    std::shared_ptr<VideoCodecCallback> vCb = videoCallback_.lock();
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
    std::shared_ptr<VideoCodecCallback> vCb = videoCallback_.lock();
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
    std::shared_ptr<VideoCodecCallback> vCb = videoCallback_.lock();
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (vCb != nullptr) {
        vCb->OnInputBufferAvailable(index, buffer);
    } else if (cb != nullptr) {
        std::shared_ptr<AVSharedMemory> memory = nullptr;
        // TODO: dma --> sharedMem
        GetBufferElem(index, buffer, memory);
        CHECK_AND_RETURN_LOG(memory != nullptr, "Get memory from caches failed");
        cb->OnInputBufferAvailable(index, memory);
    }
    Finalize();
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::shared_ptr<VideoCodecCallback> vCb = videoCallback_.lock();
    if (vCb != nullptr) {
        vCb->OnOutputBufferAvailable(index, buffer);
    }

    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        AVCodecBufferFlag flag;
        AVCodecBufferInfo info;
        std::shared_ptr<AVSharedMemory> memory = nullptr;
        ReadOutputMemory(index, info, flag, memory);
        cb->OnOutputBufferAvailable(index, info, flag, memory);
    }
    Finalize();
}

void CodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}

void CodecListenerStub::SetCallback(const std::shared_ptr<VideoCodecCallback> &callback)
{
    videoCallback_ = callback;
}

void CodecListenerStub::WaitCallbackDone()
{
    std::unique_lock<std::mutex> lock(syncMutex_);
    syncCv_.wait(lock, [this]() { return !callbackIsDoing_; });
}

int32_t CodecListenerStub::WriteInputMemory(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    std::shared_ptr<AVSharedMemory> memory = nullptr;
    GetBufferElem(index, buffer, memory);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_NO_MEMORY, "Get buffer from caches failed");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, AVCS_ERR_NO_MEMORY, "Get memory from caches failed");
    MemoryType type = buffer->memory_->GetMemoryType();
    if (type != MemoryType::SHARED_MEMORY) {
        (void)buffer->memory_->Write(memory->GetBase(), info.size, 0);
    }

    buffer->pts_ = info.presentationTimeUs;
    buffer->memory_->SetOffset(info.offset);
    buffer->memory_->SetSize(info.size);
    buffer->flag_ = static_cast<uint32_t>(flag);
    return AVCS_ERR_OK;
}

void CodecListenerStub::ReadOutputMemory(uint32_t index, AVCodecBufferInfo &info, AVCodecBufferFlag &flag,
                                         std::shared_ptr<AVSharedMemory> memory)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    GetBufferElem(index, buffer, memory);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_NO_MEMORY, "Get buffer from caches failed");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, AVCS_ERR_NO_MEMORY, "Get memory from caches failed");
    MemoryType type = buffer->memory_->GetMemoryType();
    if (type != MemoryType::SHARED_MEMORY) {
        (void)buffer->memory_->Write(memory->GetBase(), info.size, 0);
    }

    buffer->pts_ = info.presentationTimeUs;
    buffer->memory_->SetOffset(info.offset);
    buffer->memory_->SetSize(info.size);
    buffer->flag_ = static_cast<uint32_t>(flag);
    return AVCS_ERR_OK;
}

bool CodecListenerStub::WriteInputMemoryInfo(uint32_t index, MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    std::shared_ptr<AVSharedMemory> memory = nullptr;
    GetBufferElem(index, buffer, memory);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, false, "Get buffer from caches failed");
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
