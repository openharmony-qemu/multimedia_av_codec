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
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecListenerProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
CodecListenerProxy::CodecListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardCodecListener>(impl)
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

class CodecListenerProxy::CodecBufferCache : public NoCopyable {
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

void CodecListenerProxy::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteInt32(static_cast<int32_t>(errorType));
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(CodecListenerMsg::ON_ERROR, data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputFormatChanged(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    (void)AVCodecParcel::Marshalling(data, format);
    int error = Remote()->SendRequest(CodecListenerMsg::ON_OUTPUT_FORMAT_CHANGED, data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    CHECK_AND_RETURN_LOG(inputBufferCache_ != nullptr, "Input buffer cache is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteUint32(index);
    inputBufferCache_->WriteToParcel(index, buffer, data);
    int error = Remote()->SendRequest(CodecListenerMsg::ON_INPUT_BUFFER_AVAILABLE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                 std::shared_ptr<AVSharedMemory> buffer)
{
    CHECK_AND_RETURN_LOG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteUint32(index);
    data.WriteInt64(info.presentationTimeUs);
    data.WriteInt32(info.size);
    data.WriteInt32(info.offset);
    data.WriteInt32(static_cast<int32_t>(flag));
    outputBufferCache_->WriteToParcel(index, buffer, data);
    int error = Remote()->SendRequest(CodecListenerMsg::ON_OUTPUT_BUFFER_AVAILABLE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

CodecListenerCallback::CodecListenerCallback(const sptr<IStandardCodecListener> &listener)
    : listener_(listener)
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

void CodecListenerCallback::OnInputBufferAvailable(uint32_t index)
{
    (void)index;
    AVCODEC_LOGE("unsupport interface");
}

void CodecListenerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    (void)index;
    (void)info;
    (void)flag;
    AVCODEC_LOGE("unsupport interface");
}

void CodecListenerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    if (listener_ != nullptr) {
        listener_->OnInputBufferAvailable(index, buffer);
    }
}

void CodecListenerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag, std::shared_ptr<AVSharedMemory> buffer)
{
    if (listener_ != nullptr) {
        listener_->OnOutputBufferAvailable(index, info, flag, buffer);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
