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

namespace OHOS {
namespace MediaAVCodec {
class CodecListenerStub::CodecBufferCache : public NoCopyable {
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

int CodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (CodecListenerStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }

    switch (code) {
        case CodecListenerMsg::ON_ERROR: {
            int32_t errorType = data.ReadInt32();
            int32_t errorCode = data.ReadInt32();
            OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
            return AVCS_ERR_OK;
        }
        case CodecListenerMsg::ON_OUTPUT_FORMAT_CHANGED: {
            Format format;
            (void)AVCodecParcel::Unmarshalling(data, format);
            OnOutputFormatChanged(format);
            return AVCS_ERR_OK;
        }
        case CodecListenerMsg::ON_INPUT_BUFFER_AVAILABLE: {
            CHECK_AND_RETURN_RET_LOG(inputBufferCache_ != nullptr, AVCS_ERR_NO_MEMORY, "Input buffer cache is nullptr");
            uint32_t index = data.ReadUint32();
            std::shared_ptr<AVSharedMemory> memory = nullptr;
            int32_t ret = inputBufferCache_->ReadFromParcel(index, data, memory);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Read inputBuffer from parcel failed");
            OnInputBufferAvailable(index, memory);
            return AVCS_ERR_OK;
        }
        case CodecListenerMsg::ON_OUTPUT_BUFFER_AVAILABLE: {
            CHECK_AND_RETURN_RET_LOG(outputBufferCache_ != nullptr, AVCS_ERR_NO_MEMORY, "Output buffer cache is nullptr");
            uint32_t index = data.ReadUint32();
            AVCodecBufferInfo info;
            info.presentationTimeUs = data.ReadInt64();
            info.size = data.ReadInt32();
            info.offset = data.ReadInt32();
            AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());
            std::shared_ptr<AVSharedMemory> memory = nullptr;
            int32_t ret = outputBufferCache_->ReadFromParcel(index, data, memory);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Read outputBuffer from parcel failed");
            OnOutputBufferAvailable(index, info, flag, memory);
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
    if (callback_ != nullptr) {
        callback_->OnError(errorType, errorCode);
    }
}

void CodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    if (callback_ != nullptr) {
        callback_->OnOutputFormatChanged(format);
    }
}

void CodecListenerStub::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    if (callback_ != nullptr) {
        callback_->OnInputBufferAvailable(index, buffer);
    }
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                std::shared_ptr<AVSharedMemory> buffer)
{
    if (callback_ != nullptr) {
        callback_->OnOutputBufferAvailable(index, info, flag, buffer);
    }
}

void CodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}
} // namespace MediaAVCodec
} // namespace OHOS
