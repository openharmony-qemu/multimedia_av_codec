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

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecListenerStub"};
}

namespace OHOS {
namespace MediaAVCodec {
CodecListenerStub::CodecListenerStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListenerStub::~CodecListenerStub()
{
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
            uint32_t index = data.ReadUint32();
            OnInputBufferAvailable(index);
            return AVCS_ERR_OK;
        }
        case CodecListenerMsg::ON_OUTPUT_BUFFER_AVAILABLE: {
            uint32_t index = data.ReadUint32();
            AVCodecBufferInfo info;
            info.presentationTimeUs = data.ReadInt64();
            info.size = data.ReadInt32();
            info.offset = data.ReadInt32();
            AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());
            OnOutputBufferAvailable(index, info, flag);
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
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnError(errorType, errorCode);
    }
}

void CodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnOutputFormatChanged(format);
    }
}

void CodecListenerStub::OnInputBufferAvailable(uint32_t index)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnInputBufferAvailable(index);
    }
}

void CodecListenerStub::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_ptr<AVCodecCallback> cb = callback_.lock();
    if (cb != nullptr) {
        cb->OnOutputBufferAvailable(index, info, flag);
    }
}

void CodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}
} // namespace MediaAVCodec
} // namespace OHOS
