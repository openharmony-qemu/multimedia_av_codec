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
#include "avcodec_log_ex.h"
#include "avcodec_death_recipient.h"
#include "meta.h"
#include "i_codec_service.h"
#include "i_standard_codec_listener.h"
#include "i_standard_codec_service.h"
#include "nocopyable.h"
#include "instance_info.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecServiceStub : public IRemoteStub<IStandardCodecService>, public AVCodecDfxComponent, public NoCopyable {
public:
    static sptr<CodecServiceStub> Create(int32_t instanceId = INVALID_INSTANCE_ID);
    virtual ~CodecServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;

    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name, Media::Meta &callerInfo) override;
    int32_t Configure(const Format &format) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t GetChannelId(int32_t &channelId) override;
    int32_t NotifyEos() override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t SetLowPowerPlayerMode(bool isLpp) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    int32_t QueueInputBuffer(uint32_t index) override;
    int32_t QueueInputParameter(uint32_t index) override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) override;
    int32_t SetParameter(const Format &format) override;
    int32_t GetInputFormat(Format &format) override;
    int32_t GetCodecInfo(Format &format) override;
    int32_t DestroyStub() override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag) override;
#endif
    int32_t Dump(int32_t fd, const std::vector<std::u16string>& args) override;
    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer) override;
    int32_t NotifyMemoryExchange(const bool exchangeFlag) override;
    int32_t NotifyFreeze();
    int32_t NotifyActive();

private:
    CodecServiceStub();
    int32_t InitStub(int32_t instanceId = INVALID_INSTANCE_ID);
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t Init(MessageParcel &data, MessageParcel &reply);
    int32_t Configure(MessageParcel &data, MessageParcel &reply);
    int32_t Prepare(MessageParcel &data, MessageParcel &reply);
    int32_t Start(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Flush(MessageParcel &data, MessageParcel &reply);
    int32_t Reset(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t GetChannelId(MessageParcel &data, MessageParcel &reply);
    int32_t NotifyEos(MessageParcel &data, MessageParcel &reply);
    int32_t CreateInputSurface(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputSurface(MessageParcel &data, MessageParcel &reply);
    int32_t SetLowPowerPlayerMode(MessageParcel &data, MessageParcel &reply);
    int32_t QueueInputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t GetOutputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t ReleaseOutputBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t RenderOutputBufferAtTime(MessageParcel &data, MessageParcel &reply);
    int32_t SetParameter(MessageParcel &data, MessageParcel &reply);
    int32_t GetInputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t GetCodecInfo(MessageParcel &data, MessageParcel &reply);

    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(MessageParcel &data, MessageParcel &reply);
#endif
    int32_t SetCustomBuffer(MessageParcel &data, MessageParcel &reply);
    int32_t NotifyMemoryExchange(MessageParcel &data, MessageParcel &reply);
    int32_t InnerRelease();
    void NotifyMemoryRecycle(MessageParcel &data, MessageParcel &reply);
    void NotifyMemoryWriteBack(MessageParcel &data, MessageParcel &reply);
    void NotifySuspend(MessageParcel &data, MessageParcel &reply);
    void NotifyResume(MessageParcel &data, MessageParcel &reply);
    void OnActive();

    bool isServerReleased_ = false;
    std::shared_ptr<ICodecService> codecServer_ = nullptr;
    std::shared_mutex mutex_;
    sptr<IStandardCodecListener> listener_ = nullptr;
    std::atomic<bool> isMemoryRecycleFlag_{false};
    std::atomic<bool> suspended_{false};
    int32_t instanceId_ = INVALID_INSTANCE_ID;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVICE_STUB_H
