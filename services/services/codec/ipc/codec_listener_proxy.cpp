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
#include "avcodec_log_ex.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "buffer/avbuffer.h"
#include "meta/meta.h"
#include "surface_buffer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListenerProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
class CodecListenerProxy::CodecBufferCache : public AVCodecDfxComponent, public NoCopyable {
public:
    CodecBufferCache() = default;
    ~CodecBufferCache() = default;

    bool WriteToParcel(uint32_t index, const std::shared_ptr<AVBuffer> &buffer, MessageParcel &parcel)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        CacheFlag flag = CacheFlag::UPDATE_CACHE;
        if (buffer == nullptr) {
            AVCODEC_LOGD_WITH_TAG("Invalid buffer for index: %{public}u", index);
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
            return buffer->WriteToMessageParcel(parcel, false);
        }

        if (iter == caches_.end()) {
            AVCODEC_LOGD_WITH_TAG("Add cache codec buffer, index: %{public}u", index);
            caches_.emplace(index, buffer);
        } else {
            AVCODEC_LOGD_WITH_TAG("Update cache codec buffer, index: %{public}u", index);
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

    void ClearCaches(bool cleanExpiredOnly = true)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        if (cleanExpiredOnly) {
            for (auto it = caches_.begin(); it != caches_.end();) {
                if (it->second.expired()) {
                    it = caches_.erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            caches_.clear();
        }
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
    AVCODEC_LOGD_WITH_TAG("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListenerProxy::~CodecListenerProxy()
{
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    AVCODEC_LOGD_WITH_TAG("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecListenerProxy::Init()
{
    const std::string &tag = this->GetTag();
    inputBufferCache_->SetTag(tag + "[in]");
    outputBufferCache_->SetTag(tag + "[out]");
}

void CodecListenerProxy::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG_WITH_TAG(token, "Write descriptor failed!");

    data.WriteUint64(GetGeneration());
    data.WriteInt32(static_cast<int32_t>(errorType));
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_ERROR), data, reply, option);
    CHECK_AND_RETURN_LOG_WITH_TAG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputFormatChanged(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG_WITH_TAG(token, "Write descriptor failed!");

    data.WriteUint64(GetGeneration());
    (void)AVCodecParcel::Marshalling(data, format);
    CHECK_AND_RETURN_LOG_WITH_TAG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    outputBufferCache_->ClearCaches(false);
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_FORMAT_CHANGED), data,
                                      reply, option);
    CHECK_AND_RETURN_LOG_WITH_TAG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(inputBufferCache_ != nullptr, "Input buffer cache is nullptr");
    std::lock_guard<std::mutex> lock(parcelMutex_);
    MessageParcel &data = input_;
    MessageParcel &reply = output_;
    ResetParcel(data);
    ResetParcel(reply);
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG_WITH_TAG(token, "Write descriptor failed!");

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
    CHECK_AND_RETURN_LOG_WITH_TAG(ret, "InputBufferCache write parcel failed");
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_INPUT_BUFFER_AVAILABLE),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG_WITH_TAG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    std::lock_guard<std::mutex> lock(parcelMutex_);
    MessageParcel &data = input_;
    MessageParcel &reply = output_;
    ResetParcel(data);
    ResetParcel(reply);
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG_WITH_TAG(token, "Write descriptor failed!");

    uint64_t currentGeneration = GetGeneration();
    if (outputBufferGeneration_ != currentGeneration) {
        outputBufferCache_->ClearCaches();
        outputBufferGeneration_ = currentGeneration;
    }

    data.WriteUint64(outputBufferGeneration_);
    data.WriteUint32(index);
    bool ret = outputBufferCache_->WriteToParcel(index, buffer, data);
    CHECK_AND_RETURN_LOG_WITH_TAG(ret, "OutputBufferCache write parcel failed");
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_AVAILABLE),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG_WITH_TAG(error == AVCS_ERR_OK, "Send request failed");
}

void CodecListenerProxy::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    AVCODEC_LOGI("LowPowerPlayer Send request OnOutputBufferBinded Enter");
    CHECK_AND_RETURN_LOG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "LowpowerPlayer Write descriptor failed!");
    data.WriteUint64(GetGeneration());
    data.WriteUint32(bufferMap.size());
    for (const auto& buffer : bufferMap) {
        if (!data.WriteUint32((buffer.first))) {
            AVCODEC_LOGE("LowPowerPlayer Write Data Key Failed");
        }
        if (buffer.second != nullptr) {
            bool res = buffer.second->WriteToMessageParcel(data);
            AVCODEC_LOGI("LowPowerPlayer WriteToMessageParcel: %{public}u", static_cast<uint32_t>(res));
        }
    }
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_BINDED),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "LowPowerPlayer Send request OnOutputBufferBinded failed");
}

void CodecListenerProxy::OnOutputBufferUnbinded()
{
    CHECK_AND_RETURN_LOG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool token = data.WriteInterfaceToken(CodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "LowpowerPlayer Write descriptor failed!");
    data.WriteUint64(GetGeneration());
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_UN_BINDED),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "LowPowerPlayer Send request OnOutputBufferUnbinded failed");
}

bool CodecListenerProxy::InputBufferInfoFromParcel(uint32_t index, AVCodecBufferInfo &info, AVCodecBufferFlag &flag,
                                                   MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer = inputBufferCache_->FindBufferFromIndex(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer != nullptr, false, "Input buffer in cache is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer->meta_ != nullptr, false, "buffer meta is nullptr");
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

void CodecListenerProxy::SetOutputBufferRenderTimestamp(uint32_t index, int64_t renderTimestampNs)
{
    std::shared_ptr<AVBuffer> buffer = outputBufferCache_->FindBufferFromIndex(index);
    if (buffer == nullptr) {
        return;
    }
    buffer->meta_->SetData(Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP, renderTimestampNs);
}

void CodecListenerProxy::ClearListenerCache()
{
    inputBufferCache_->ClearCaches();
    outputBufferCache_->ClearCaches();
}

void CodecListenerProxy::ResetParcel(MessageParcel &parcel)
{
    parcel.ClearFileDescriptor();
    parcel.RewindRead(0);
    parcel.RewindWrite(0);
    parcel.SetDataSize(0);
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

void CodecListenerCallback::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    if (listener_ != nullptr) {
        listener_->OnOutputBufferBinded(bufferMap);
    }
}
void CodecListenerCallback::OnOutputBufferUnbinded()
{
    if (listener_ != nullptr) {
        listener_->OnOutputBufferUnbinded();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
