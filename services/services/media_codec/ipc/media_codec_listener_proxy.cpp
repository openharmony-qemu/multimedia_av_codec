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

#include "media_codec_listener_proxy.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecListenerProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
MediaCodecListenerProxy::MediaCodecListenerProxy(const sptr<IRemoteObject> &impl) :
    IRemoteProxy<IStandardMediaCodecListener>(impl)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecListenerProxy::~MediaCodecListenerProxy()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MediaCodecListenerProxy::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(MediaCodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteUint64(GetGeneration());
    data.WriteInt32(static_cast<int32_t>(errorType));
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_ERROR), data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void MediaCodecListenerProxy::OnStreamChanged(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(MediaCodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    data.WriteUint64(GetGeneration());
    (void)AVCodecParcel::Marshalling(data, format);
    CHECK_AND_RETURN_LOG(outputBufferCache_ != nullptr, "Output buffer cache is nullptr");
    outputBufferCache_->ClearCaches();
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_STREAM_CHANGED), data,
                                      reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

void MediaCodecListenerProxy::onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG(buffer != nullptr, "Buffer is nullptr");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool token = data.WriteInterfaceToken(MediaCodecListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Write descriptor failed!");

    int32_t ret = buffer->WriteToMessageParcel(data);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Buffer write parcel failed");
    int error = Remote()->SendRequest(static_cast<uint32_t>(CodecListenerInterfaceCode::ON_SURFACE_MODE_DATA),
                                      data, reply, option);
    CHECK_AND_RETURN_LOG(error == AVCS_ERR_OK, "Send request failed");
}

MediaCodecListenerCallback::MediaCodecListenerCallback(const sptr<IStandardMediaCodecListener> &listener) : listener_(listener)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecListenerCallback::~MediaCodecListenerCallback()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MediaCodecListenerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void MediaCodecListenerCallback::OnStreamChanged(const Format &format)
{
    if (listener_ != nullptr) {
        listener_->OnStreamChanged(format);
    }
}
void MediaCodecListenerCallback::onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer)
{
    if (listener_ != nullptr) {
        listener_->onSurfaceModeData(buffer);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
