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
#ifndef CODECBASE_H
#define CODECBASE_H

#include <string>
#include "avcodec_common.h"
#include "avsharedmemorybase.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecBase {
public:
    CodecBase() = default;
    virtual ~CodecBase() = default;
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetParameter(const Format& format) = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag);
    virtual int32_t QueueInputBuffer(uint32_t index);
    virtual int32_t ReleaseOutputBuffer(uint32_t index) = 0;

    virtual int32_t NotifyEos();
    virtual sptr<Surface> CreateInputSurface();
    virtual int32_t SetOutputSurface(sptr<Surface> surface);
    virtual int32_t RenderOutputBuffer(uint32_t index);
    virtual int32_t SignalRequestIDRFrame();
    virtual int32_t GetInputFormat(Format& format);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECBASE_H