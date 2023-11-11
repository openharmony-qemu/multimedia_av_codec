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
#include "buffer/avbuffer.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecListenerStub"};
}

namespace OHOS {
namespace MediaAVCodec {
class MediaCodecListenerStub::MediaCodecBufferCache : public NoCopyable {
public:
    MediaCodecBufferCache() = default;
    ~MediaCodecBufferCache() = default;

    void ReadFromParcel(MessageParcel &parcel, std::shared_ptr<Media::AVBuffer> &buffer)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CacheFlag flag = static_cast<CacheFlag>(parcel.ReadUint8());
        uint64_t index = parcel.ReadUint64();
        auto iter = caches_.find(index);
        if (flag == CacheFlag::HIT_CACHE) {
            if (iter == caches_.end()) {
                AVCODEC_LOGE("Mark hit cache, but can find the index's cache, index: %{public}llu", index);
                return;
            }
            buffer = iter->second;
            buffer->pts_ = parcel.ReadInt64();
            buffer->memory_->SetSize(parcel.ReadInt32());
            buffer->memory_->SetOffset(parcel.ReadInt32());
            buffer->flag_ = parcel.ReadInt32();
            return;
        }

        buffer = Media::AVBuffer::CreateAVBuffer(parcel);
        CHECK_AND_RETURN_LOG(buffer != nullptr, "Read buffer from parcel failed");
        if (iter == caches_.end()) {
            AVCODEC_LOGI("Add cache, index: %{public}u", index);
            caches_.emplace(index, buffer);
        } else {
            iter->second = buffer;
            AVCODEC_LOGI("Update cache, index: %{public}u", index);
        }
        return;
    }

    void ClearCaches()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        caches_.clear();
    }

    bool FindBufferIndex(uint64_t &index)
    {
        auto iter = caches_.find(index);
        if (iter == caches_.end()) {
            return false;
        }
        return true;
    }

private:
    std::mutex mutex_;
    enum class CacheFlag : uint8_t {
        HIT_CACHE = 1,
        UPDATE_CACHE,
    };
    std::unordered_map<uint64_t, std::shared_ptr<Media::AVBuffer>> caches_;
};

MediaCodecListenerStub::MediaCodecListenerStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecListenerStub::~MediaCodecListenerStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int MediaCodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(MediaCodecListenerStub::GetDescriptor() == remoteDescriptor, AVCS_ERR_INVALID_OPERATION,
                             "Invalid descriptor");

    std::unique_lock<std::mutex> lock(syncMutex_, std::try_to_lock);
    CHECK_AND_RETURN_RET_LOG(lock.owns_lock(), AVCS_ERR_OK, "abandon message");
    switch (code) {
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_ERROR): {
            int32_t errorType = data.ReadInt32();
            int32_t errorCode = data.ReadInt32();
            OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_STREAM_CHANGED): {
            Format format;
            (void)AVCodecParcel::Unmarshalling(data, format);
            outputBufferCache_->ClearCaches();
            OnStreamChanged(format);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_SURFACE_MODE_DATA): {
            std::shared_ptr<Media::AVBuffer> buffer = nullptr;
            outputBufferCache_->ReadFromParcel(data, buffer);
            SurfaceModeOnBufferFilled(buffer);
            return AVCS_ERR_OK;
        }
        default: {
            AVCODEC_LOGE("Default case, please check codec listener stub");
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
}

void MediaCodecListenerStub::OnStreamChanged(const Format &format)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnStreamChanged(format);
    }
}

void MediaCodecListenerStub::SurfaceModeOnBufferFilled(std::shared_ptr<Media::AVBuffer> buffer)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->SurfaceModeOnBufferFilled(buffer);
    }
}

void MediaCodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}

bool MediaCodecListenerStub::FindBufferFromIndex(uint64_t index, std::shared_ptr<Media::AVBuffer> buffer)
{
    return outputBufferCache_->FindBufferIndex(index);
}
} // namespace MediaAVCodec
} // namespace OHOS
