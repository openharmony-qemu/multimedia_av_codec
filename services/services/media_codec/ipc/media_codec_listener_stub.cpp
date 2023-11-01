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
            OnStreamChanged(format);
            return AVCS_ERR_OK;
        }
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_SURFACE_MODE_DATA): {
            std::shared_ptr<Media::AVBuffer> buffer = Media::AVBuffer::CreateAVBuffer(data, true);
            onSurfaceModeData(buffer);
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

void MediaCodecListenerStub::onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->onSurfaceModeData(buffer);
    }
}

void MediaCodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}
} // namespace MediaAVCodec
} // namespace OHOS
