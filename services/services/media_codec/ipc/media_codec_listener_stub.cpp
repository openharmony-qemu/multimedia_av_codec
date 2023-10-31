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

#include "media_codec_listener_stub.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecListenerStub"};
}

namespace OHOS {
namespace MediaAVCodec {
class MediaCodecListenerStub::MediaCodecBufferCache : public NoCopyable {
public:
    MediaCodecBufferCache() = default;
    ~MediaCodecBufferCache() = default;

    void ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVSharedMemory> &memory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = caches_.find(index);
        CacheFlag flag = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}u", index);
                return;
            }
            memory = iter->second;
            return;
        }

        if (flag == CacheFlag::UPDATE_CACHE) {
            memory = ReadAVSharedMemoryFromParcel(parcel);
            CHECK_AND_RETURN_LOG(memory != nullptr, "Read memory from parcel failed");

            if (iter == caches_.end()) {
                AVCODEC_LOGI("Add cache, index: %{public}u", index);
                caches_.emplace(index, memory);
            } else {
                iter->second = memory;
                AVCODEC_LOGI("Update cache, index: %{public}u", index);
            }
            return;
        }

        // invalidate cache flag
        if (iter != caches_.end()) {
            iter->second = nullptr;
            caches_.erase(iter);
        }
        memory = nullptr;
        AVCODEC_LOGD("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag);
        return;
    }

    void ClearCaches()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        caches_.clear();
    }

private:
    std::mutex mutex_;
    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };
    std::unordered_map<uint32_t, std::shared_ptr<AVSharedMemory>> caches_;
};

MediaCodecListenerStub::MediaCodecListenerStub()
{
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<MediaCodecBufferCache>();
    }

    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<MediaCodecBufferCache>();
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecListenerStub::~MediaCodecListenerStub()
{
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int MediaCodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(MediaCodecListenerStub::GetDescriptor() == remoteDescriptor, AVCS_ERR_INVALID_OPERATION,
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
            std::shared_ptr<AVSharedMemory> memory = nullptr;
            inputBufferCache_->ReadFromParcel(index, data, memory);
            OnInputBufferAvailable(index, memory);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_AVAILABLE): {
            uint32_t index = data.ReadUint32();
            AVCodecBufferInfo info { data.ReadInt64(), data.ReadInt32(), data.ReadInt32() };
            AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());
            std::shared_ptr<AVSharedMemory> memory = nullptr;
            outputBufferCache_->ReadFromParcel(index, data, memory);
            OnOutputBufferAvailable(index, info, flag, memory);
            return AVCS_ERR_OK;
        }
        default: {
            AVCODEC_LOGE("Default case, please check codec listener stub");
            Finalize();
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void MediaCodecListenerStub::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnError(errorType, errorCode);
    }
    Finalize();
}

void MediaCodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnOutputFormatChanged(format);
    }
    Finalize();
}

void MediaCodecListenerStub::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnInputBufferAvailable(index, buffer);
    }
    Finalize();
}

void MediaCodecListenerStub::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                std::shared_ptr<AVSharedMemory> buffer)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnOutputBufferAvailable(index, info, flag, buffer);
    }
    Finalize();
}

void MediaCodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}

void MediaCodecListenerStub::WaitCallbackDone()
{
    std::unique_lock<std::mutex> lock(syncMutex_);
    syncCv_.wait(lock, [this]() { return !callbackIsDoing_; });
}

bool MediaCodecListenerStub::CheckGeneration(uint64_t messageGeneration) const
{
    return messageGeneration >= GetGeneration();
}

void MediaCodecListenerStub::Finalize()
{
    callbackIsDoing_ = false;
    syncCv_.notify_one();
}
} // namespace MediaAVCodec
} // namespace OHOS
