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

#ifndef I_MEDIA_CODEC_SERVICE_H
#define I_MEDIA_CODEC_SERVICE_H

#include <string>
#include "refbase.h"
#include "avbuffer_queue_producer.h"

namespace OHOS {
namespace MediaAVCodec {
class IMediaCodecService {
public:
    virtual ~IMediaCodecService() = default;

    virtual int32_t Init(bool isEncoder, bool isMimeType, const std::string &name) = 0;
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecMediaCodecCallback> &callback) = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t SetParameter(const Format &format) = 0;
    virtual sptr<Media::AVBufferQueueProducer> GetInputBufferQueue() = 0;
    virtual int32_t SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue) = 0;
    virtual sptr<Surface> CreateInputSurface() = 0;
    virtual int32_t SetOutputSurface(sptr<Surface> surface) = 0;
    virtual int32_t NotifyEos() = 0;
    virtual int32_t SurfaceModeReturnData(std::shared_ptr<Meida::AVBuffer> buffer, bool available) = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // I_CODEC_SERVICE_H