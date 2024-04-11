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

#include "codec_listener_proxy.h"
#include <shared_mutex>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"
#include "buffer/avbuffer.h"
#include "meta/meta.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListenerProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
class CodecListenerProxy::CodecBufferCache : public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    bool WriteToParcel(uint32_t index, const std::shared_ptr<AVBuffer> &buffer, MessageParcel &parcel)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        CacheFlag flag = CacheFlag::UPDATE_CACHE;
        if (buffer == nullptr) {
            AVCODEC_LOGD("Invalid buffer for index: %{public}u", index);
            flag = CacheFlag::INVALIDATE_CACHE;
            parcel.WriteUint8(static_cast<uint8_t>(flag));
            auto iter = caches_.find(index);
            if (iter != caches_.end()) {
                iter->second = std::shared_ptr<AVBuffer>(nullptr);
                caches_.erase(iter);
            }
            return true;
        }

        auto iter = caches_.find(index);
        if (iter != caches_.end() && iter->second.lock() == buffer) {
            flag = CacheFlag::HIT_CACHE;
            parcel.WriteUint8(static_cast<uint8_t>(flag));
            return buffer->WriteToMessageParcel(parcel);
        }

        if (iter == caches_.end()) {
            AVCODEC_LOGD("Add cache codec buffer, index: %{public}u", index);
            caches_.emplace(index, buffer);
        } else {
            AVCODEC_LOGD("Update cache codec buffer, index: %{public}u", index);
            iter->second = buffer;
        }

        parcel.WriteUint8(static_cast<uint8_t>(flag));

        return buffer->WriteToMessageParcel(parcel);
    }

    std::shared_ptr<AVBuffer> FindBufferFromIndex(uint32_t index)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        if (iter != caches_.end()) {
            return iter->second.lock();
        }
        return nullptr;
    }

    void ClearCaches()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        caches_.clear();
    }

private:
    std::shared_mutex mutex_;
    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };

    std::unordered_map<uint32_t, std::weak_ptr<AVBuffer>> caches_;
};

CodecListenerProxy::CodecListenerProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStandardCodecListener>(impl)
{
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<CodecBufferCache>();
    }
    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<CodecBufferCache>();
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListenerProxy::~CodecListenerProxy()
{
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecListenerProxy::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteUint64(GetGeneration());
    data.WriteInt32(static_cast<int32_t>(errorType));
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_ERROR), data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputFormatChanged(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteUint64(GetGeneration());
    (void)AVCodecParcel::Marshalling(data, format);
    CHECK_AND_RETURN_LOG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    outputBufferCache_->ClearCaches();
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_FORMAT_CHANGED), data,
                                      reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG(inputBufferCache_ != nullptr, "Input buffer cache is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    uint64_t currentGeneration = GetGeneration();
    if (inputBufferGeneration_ != currentGeneration) {
        inputBufferCache_->ClearCaches();
        inputBufferGeneration_ = currentGeneration;
    }

    data.WriteUint64(inputBufferGeneration_);
    data.WriteUint32(index);
    if (buffer != nullptr && buffer->meta_ != nullptr) {
        buffer->meta_->Clear();
    }
    bool ret = inputBufferCache_->WriteToParcel(index, buffer, data);
    CHECK_AND_RETURN_LOG(ret, "InputBufferCache write parcel failed");
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_INPUT_BUFFER_AVAILABLE),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    uint64_t currentGeneration = GetGeneration();
    if (outputBufferGeneration_ != currentGeneration) {
        outputBufferCache_->ClearCaches();
        outputBufferGeneration_ = currentGeneration;
    }

    data.WriteUint64(outputBufferGeneration_);
    data.WriteUint32(index);
    bool ret = outputBufferCache_->WriteToParcel(index, buffer, data);
    CHECK_AND_RETURN_LOG(ret, "OutputBufferCache write parcel failed");
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_AVAILABLE),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

bool CodecListenerProxy::InputBufferInfoFromParcel(uint32_t index, AVCodecBufferInfo &info, AVCodecBufferFlag &flag,
                                                   MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer = inputBufferCache_->FindBufferFromIndex(index);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, false, "Input buffer in cache is nullptr");
    CHECK_AND_RETURN_RET_LOG(buffer->meta_ != nullptr, false, "buffer meta is nullptr");
    if (buffer->memory_ == nullptr) {
        return buffer->meta_->FromParcel(data);
    }
    uint32_t flagTemp = 0;
    bool ret = data.ReadInt64(info.presentationTimeUs) && data.ReadInt32(info.offset) && data.ReadInt32(info.size) &&
               data.ReadUint32(flagTemp);
    flag = static_cast<AVCodecBufferFlag>(flagTemp);
    buffer->pts_ = info.presentationTimeUs;
    buffer->flag_ = flag;
    buffer->memory_->SetOffset(info.offset);
    buffer->memory_->SetSize(info.size);
    return ret && buffer->meta_->FromParcel(data);
}

void CodecListenerProxy::ClearListenerCache()
{
    inputBufferCache_->ClearCaches();
    outputBufferCache_->ClearCaches();
}

CodecListenerCallback::CodecListenerCallback(const sptr<IStandardCodecListener> &listener) : listener_(listener)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListenerCallback::~CodecListenerCallback()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecListenerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void CodecListenerCallback::OnOutputFormatChanged(const Format &format)
{
    if (listener_ != nullptr) {
        listener_->OnOutputFormatChanged(format);
    }
}

void CodecListenerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (listener_ != nullptr) {
        listener_->OnInputBufferAvailable(index, buffer);
    }
}

void CodecListenerCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (listener_ != nullptr) {
        listener_->OnOutputBufferAvailable(index, buffer);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
