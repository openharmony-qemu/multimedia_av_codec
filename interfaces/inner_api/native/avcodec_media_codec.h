/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef MEDIA_AVCODEC_MEDIA_CODEC_H
#define MEDIA_AVCODEC_MEDIA_CODEC_H

#include <string>
#include <refbase.h>

#include "codec_plugin.h"
#include "plugin_meta.h"
#include "plugin_types.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media::Plugin;

class AVCodecMediaCodec {
public:
    virtual ~AVCodecVideoDecoder() = default;

    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback) = 0;
    virtual int32_t Configure(const Format &format) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t GetOutputFormat(Format &format) = 0;
    virtual int32_t SetParameter(const Format &format) = 0;
    virtual int32_t GetInputBufferQueue(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    virtual int32_t SetOutputBufferQueue(uint32_t index, bool render) override;
    virtual int32_t VideoEncoderGetSurface(sptr<Surface> surface) = 0;
    virtual int32_t VideoDecoderSetSurface(sptr<Surface> surface) = 0;
    virtual int32_t VideoEncoderNotifyEndOfStream() = 0;
    virtual int32_t VideoReturnSurfacemodeData() = 0;
};

class __attribute__((visibility("default"))) MediaCodecFactory {
public:
#ifdef UNSUPPORT_CODEC
    static std::shared_ptr<AVCodecVideoDecoder> CreateByMime(bool isEncoder, const std::string &mime)
    {
        (void)mime;
        return nullptr;
    }

    static std::shared_ptr<AVCodecVideoDecoder> CreateByName(const std::string &name)
    {
        (void)name;
        return nullptr;
    }
#else
    static std::shared_ptr<AVCodecMediaCodec> CreateByMime(bool isEncoder, const std::string &mime);
    static std::shared_ptr<AVCodecMediaCodec> CreateByName(const std::string &name);
#endif
private:
    MediaCodecFactory() = default;
    ~MediaCodecFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_AVCODEC_MEDIA_CODEC_H
