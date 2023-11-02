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

#ifndef MEDIA_CODEC_SERVICE_PROXY_H
#define MEDIA_CODEC_SERVICE_PROXY_H

#include "i_standard_media_codec_service.h"
#include "nocopyable.h"
#include "avbuffer_queue_producer.h"


namespace OHOS {
namespace MediaAVCodec {
class MediaCodecServiceProxy : public IRemoteProxy<IStandardMediaCodecService>, public NoCopyable {
public:
    explicit MediaCodecServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~MediaCodecServiceProxy();

    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;

    int32_t Init(bool isEncoder, bool isMimeType, const std::string &name) override;
    int32_t Configure(const Format &format) override;
    int32_t Start() override;
    int32_t Prepare() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t GetOutputFormat(Format &format) override;
    int32_t SetParameter(const Format &format) override;
    sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() override;
    int32_t SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue) override;
    sptr<Surface> CreateInputSurface() override;
    int32_t SetOutputSurface(sptr<Surface> surface) override;
    int32_t NotifyEos() override;
    int32_t SurfaceModeReturnData(uint64_t index, bool available) override;

    int32_t DestroyStub() override;

private:
    static inline BrokerDelegator<MediaCodecServiceProxy> delegator_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVICE_PROXY_H
