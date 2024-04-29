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

#ifndef CODEC_SERVICE_STUB_H
#define CODEC_SERVICE_STUB_H

#include <map>
#include <shared_mutex>
#include "avcodec_death_recipient.h"
#include "codec_server.h"
#include "i_standard_codec_listener.h"
#include "i_standard_codec_service.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecServiceStub : public IRemoteStub<IStandardCodecService>, public NoCopyable {
public:
    static sptr<CodecServiceStub> Create();
    virtual ~CodecServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    using CodecStubFunc = int32_t(CodecServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo) override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t NotifyEos() override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t QueueInputParameter(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetInputFormat(Format &format) override;

    int32_t DestroyStub() override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag) override;
#endif
    int32_t DumpInfo(int32_t fd);

private:
    CodecServiceStub();
    int32_t InitStub();
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t Init(MessageParcel &data, MessageParcel &reply);
    int32_t Configure(MessageParcel &data, MessageParcel &reply);
    int32_t Start(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Flush(MessageParcel &data, MessageParcel &reply);
    int32_t Reset(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t NotifyEos(MessageParcel &data, MessageParcel &reply);
    int32_t CreateInputSurface(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputSurface(MessageParcel &data, MessageParcel &reply);
    int32_t QueueInputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t GetOutputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t ReleaseOutputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t SetParameter(MessageParcel &data, MessageParcel &reply);
    int32_t GetInputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(MessageParcel &data, MessageParcel &reply);
#endif
    int32_t InnerRelease();

    std::shared_ptr<ICodecService> codecServer_ = nullptr;
    std::map<uint32_t, CodecStubFunc> recFuncs_;
    std::shared_mutex mutex_;
    sptr<IStandardCodecListener> listener_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVICE_STUB_H
