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

#ifndef CODEC_SERVICE_PROXY_H
#define CODEC_SERVICE_PROXY_H

#include "avcodec_log.h"
#include "codec_listener_stub.h"
#include "i_standard_codec_service.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecServiceProxy : public IRemoteProxy<IStandardCodecService>, public NoCopyable {
public:
    explicit CodecServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~CodecServiceProxy();

    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name, Media::Meta &callerInfo) override;
    int32_t Configure(const Format &format) override;
    int32_t Prepare() override;
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
    int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetInputFormat(Format &format) override;

    int32_t DestroyStub() override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag) override;
#endif
    void SetListener(const sptr<CodecListenerStub> &listener);
    void InitLabel(const uint64_t uid);
    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer) override;

private:
    static inline BrokerDelegator<CodecServiceProxy> delegator_;
    sptr<CodecListenerStub> listener_;

    const OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecServiceProxy"};
    std::string tag_ = "";
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVICE_PROXY_H
