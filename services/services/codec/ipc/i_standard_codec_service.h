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

#ifndef I_STANDARD_CODEC_SERVICE_H
#define I_STANDARD_CODEC_SERVICE_H

#include "av_codec_service_ipc_interface_code.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "buffer/avsharedmemory.h"
#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "surface.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"
namespace OHOS {
namespace MediaAVCodec {
class IStandardCodecService : public IRemoteBroker {
public:
    virtual ~IStandardCodecService() = default;

    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;

    virtual int32_t Init(AVCodecType type, bool isMimeType, const std::string &name, Media::Meta &callerInfo) = 0;
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t NotifyEos() = 0;
    virtual sptr<Surface> CreateInputSurface() = 0;
    virtual int32_t SetOutputSurface(sptr<Surface> surface) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index) = 0;
    virtual int32_t QueueInputParameter(uint32_t index) = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render) = 0;
    virtual int32_t SetParameter(const Format &format) = 0;
    virtual int32_t GetInputFormat(Format &format) = 0;
    virtual int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        (void)keySession;
        (void)svpFlag;
        return 0;
    }

    virtual int32_t DestroyStub() = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardCodecService");
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // I_STANDARD_CODEC_SERVICE_H
