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
#include "avcodec_trace.h"
#include "buffer/avsharedmemorybase.h"
#include "meta/meta.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListenerStub"};
constexpr uint8_t LOG_FREQ = 10;
const int32_t MAX_SIZE = 100;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class CodecListenerStub::CodecBufferCache : public AVCodecDfxComponent, public NoCopyable {
public:
    explicit CodecBufferCache(bool isOutput) : isOutput_(isOutput) {}
    ~CodecBufferCache() = default;

    bool ReadFromParcel(uint32_t index, MessageParcel &parcel, std::shared_ptr<AVBuffer> &buffer)
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        auto iter = caches_.find(index);
        flag_ = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag_ == CacheFlag::HIT_CACHE && iter == caches_.end()) {
            AVCODEC_LOGW_WITH_TAG("Mark hit cache, but can find the index's cache, index: %{public}u", index);
            flag_ = CacheFlag::UPDATE_CACHE;
        }
        if (flag_ == CacheFlag::HIT_CACHE) {
            isOutput_ ? HitOutputCache(iter->second, parcel) : HitInputCache(iter->second, parcel);
            buffer = iter->second;
            return true;
        }
        if (flag_ == CacheFlag::UPDATE_CACHE) {
            isOutput_ ? UpdateOutputCache(buffer, parcel) : UpdateInputCache(buffer, parcel);
            if (iter == caches_.end()) {
                caches_.emplace(index, buffer);
            } else {
                iter->second = buffer;
            }
            return true;
        }
        // invalidate cache flag_
        if (iter != caches_.end()) {
            caches_.erase(iter);
        }
        AVCODEC_LOGE_WITH_TAG("Invalidate cache for index: %{public}u, flag: %{public}hhu", index, flag_);
        return false;
    }

    void GetBuffer(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (isFirstSend_) {
            AVCODEC_LOGI_WITH_TAG("first send buffer to service");
            isFirstSend_ = false;
        }
        auto iter = caches_.find(index);
        if (iter == caches_.end()) {
            AVCODEC_LOGE_WITH_TAG("Get cache failed, index: %{public}u", index);
            return;
        }
        buffer = iter->second;
    }

    void ClearCaches()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        isFirstReceive_ = true;
        isFirstSend_ = true;
        caches_.clear();
    }

    void FlushCaches()
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        isFirstReceive_ = true;
        isFirstSend_ = true;
    }

private:
    void HitInputCache(std::shared_ptr<AVBuffer> &buffer, MessageParcel &parcel)
    {
        CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "buffer is nullptr");
        bool isReadSuc = buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG_WITH_TAG(isReadSuc, "Read buffer from parcel failed");
        buffer->flag_ = 0;
        if (buffer->memory_ != nullptr) {
            buffer->memory_->SetOffset(0);
            buffer->memory_->SetSize(0);
        }
        if (isFirstReceive_) {
            AVCODEC_LOGI_WITH_TAG("first receive buffer from service");
            isFirstReceive_ = false;
        }
    }

    void HitOutputCache(std::shared_ptr<AVBuffer> &buffer, MessageParcel &parcel)
    {
        CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "buffer is nullptr");
        bool isReadSuc = buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG_WITH_TAG(isReadSuc, "Read buffer from parcel failed");
        if (isFirstReceive_) {
            AVCODEC_LOGI_WITH_TAG("first receive buffer from service");
            isFirstReceive_ = false;
        }
    }

    void UpdateInputCache(std::shared_ptr<AVBuffer> &buffer, MessageParcel &parcel)
    {
        buffer = AVBuffer::CreateAVBuffer();
        bool isReadSuc = (buffer != nullptr) && buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG_WITH_TAG(isReadSuc, "Create buffer from parcel failed");
        buffer->flag_ = 0;
        if (buffer->memory_ != nullptr) {
            buffer->memory_->SetOffset(0);
            buffer->memory_->SetSize(0);
        }
        if (isFirstReceive_) {
            AVCODEC_LOGI_WITH_TAG("first receive buffer from service");
            isFirstReceive_ = false;
        }
    }

    void UpdateOutputCache(std::shared_ptr<AVBuffer> &buffer, MessageParcel &parcel)
    {
        buffer = AVBuffer::CreateAVBuffer();
        bool isReadSuc = (buffer != nullptr) && buffer->ReadFromMessageParcel(parcel);
        CHECK_AND_RETURN_LOG_WITH_TAG(isReadSuc, "Create buffer from parcel failed");
        if (isFirstReceive_) {
            AVCODEC_LOGI_WITH_TAG("first receive buffer from service");
            isFirstReceive_ = false;
        }
    }

    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
        INVALIDATE_CACHE,
    };
    bool isOutput_ = false;
    bool isFirstReceive_ = true;
    bool isFirstSend_ = true;
    CacheFlag flag_ = CacheFlag::INVALIDATE_CACHE;
    std::shared_mutex mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> caches_;
};

CodecListenerStub::CodecListenerStub()
{
    if (inputBufferCache_ == nullptr) {
        inputBufferCache_ = std::make_unique<CodecBufferCache>(false);
    }

    if (outputBufferCache_ == nullptr) {
        outputBufferCache_ = std::make_unique<CodecBufferCache>(true);
    }
    AVCODEC_LOGD_WITH_TAG("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListenerStub::~CodecListenerStub()
{
    inputBufferCache_ = nullptr;
    outputBufferCache_ = nullptr;
    AVCODEC_LOGD_WITH_TAG("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecListenerStub::Init()
{
    const std::string &tag = this->GetTag();
    inputBufferCache_->SetTag(tag + "[in]");
    outputBufferCache_->SetTag(tag + "[out]");
}

int CodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CodecListenerStub::GetDescriptor() == remoteDescriptor,
                                      AVCS_ERR_INVALID_OPERATION, "Invalid descriptor");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(inputBufferCache_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                                      "inputBufferCache is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(outputBufferCache_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                                      "outputBufferCache is nullptr");

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(syncMutex_ != nullptr, AVCS_ERR_INVALID_OPERATION, "sync mutex is nullptr");
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
            return OnRequestExtras(code, data, reply, option);
        }
    }
}

int CodecListenerStub::OnRequestExtras(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    switch (code) {
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_BINDED): {
            OnOutputBufferBinded(data);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_BUFFER_UN_BINDED): {
            OnOutputBufferUnbinded(data);
            return AVCS_ERR_OK;
        }
        default: {
            AVCODEC_LOGE_WITH_TAG("Default case, please check codec listener stub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void CodecListenerStub::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::shared_ptr<MediaCodecCallback> vCb = callback_.lock();
    if (vCb != nullptr) {
        vCb->OnError(errorType, errorCode);
        return;
    }
}

void CodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    std::shared_ptr<MediaCodecCallback> vCb = callback_.lock();
    if (vCb != nullptr) {
        vCb->OnOutputFormatChanged(format);
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

void CodecListenerStub::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    (void)bufferMap;
}
void CodecListenerStub::OnOutputBufferUnbinded()
{
}

void CodecListenerStub::OnInputBufferAvailable(uint32_t index, MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer;
    std::shared_ptr<MediaCodecCallback> mediaCb = callback_.lock();
    if (mediaCb != nullptr) {
        bool ret = inputBufferCache_->ReadFromParcel(index, data, buffer);
        CHECK_AND_RETURN_LOG_WITH_TAG(ret, "read from parel failed");
        mediaCb->OnInputBufferAvailable(index, buffer);
        return;
    }
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer;
    std::shared_ptr<MediaCodecCallback> mediaCb = callback_.lock();
    if (mediaCb != nullptr) {
        bool ret = outputBufferCache_->ReadFromParcel(index, data, buffer);
        CHECK_AND_RETURN_LOG_WITH_TAG(ret, "read from parel failed");
        mediaCb->OnOutputBufferAvailable(index, buffer);
        return;
    }
}

void CodecListenerStub::OnOutputBufferBinded(MessageParcel &data)
{
    std::shared_ptr<MediaCodecCallback> mediaCb = callback_.lock();
    if (mediaCb != nullptr) {
        std::map<uint32_t, sptr<SurfaceBuffer>> bufferMap;
        const MediaAVCodec::Format format;
        uint32_t size = data.ReadUint32();
        if (size > MAX_SIZE) {
            bufferMap.clear();
            return;
        }
        for (uint32_t i = 0; i < size; i++) {
            uint32_t key = data.ReadUint32();
            sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
            surfaceBuffer->ReadFromMessageParcel(data);
            bufferMap.emplace(key, surfaceBuffer);
        }
        AVCODEC_LOGI("LowPowerPlayer CodecListenerStub OnOutputBufferBinded Write Map Success");
        mediaCb->OnOutputBufferBinded(bufferMap);
        return;
    }
}

void CodecListenerStub::OnOutputBufferUnbinded(MessageParcel &data)
{
    AVCODEC_LOGI("LowPowerPlayer CodecListenerStub OnOutputBufferUnbinded Enter");
    std::shared_ptr<MediaCodecCallback> mediaCb = callback_.lock();
    CHECK_AND_RETURN_LOG(mediaCb != nullptr, "mediaCb is nullptr");
    mediaCb->OnOutputBufferUnbinded();
}

void CodecListenerStub::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    callback_ = callback;
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

bool CodecListenerStub::WriteInputBufferToParcel(uint32_t index, MessageParcel &data)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    inputBufferCache_->GetBuffer(index, buffer);
    CHECK_AND_RETURN_RET_LOGD_WITH_TAG(buffer != nullptr, false, "Get buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer->meta_ != nullptr, false, "Get buffer meta is nullptr");
    if (buffer->memory_ == nullptr) {
        return buffer->meta_->ToParcel(data);
    }
    return data.WriteInt64(buffer->pts_) && data.WriteInt32(buffer->memory_->GetOffset()) &&
           data.WriteInt32(buffer->memory_->GetSize()) && data.WriteUint32(buffer->flag_) &&
           buffer->meta_->ToParcel(data);
}

bool CodecListenerStub::WriteOutputBufferToParcel(uint32_t index, MessageParcel &data)
{
    (void)data;
    std::shared_ptr<AVBuffer> buffer = nullptr;
    outputBufferCache_->GetBuffer(index, buffer);
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

void CodecListenerStub::SetNeedListen(const bool needListen)
{
    needListen_ = needListen;
}
} // namespace MediaAVCodec
} // namespace OHOS
