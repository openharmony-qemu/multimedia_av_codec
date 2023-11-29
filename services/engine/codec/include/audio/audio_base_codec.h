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

#ifndef BASE_CODER_PLUGIN
#define BASE_CODER_PLUGIN

#include <string>

#include "audio_buffer_info.h"
#include "av_codec_base_factory.h"
#include "meta/format.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioBaseCodec : public AVCodecBaseFactory<AudioBaseCodec, std::string>, public NoCopyable {
public:
    AudioBaseCodec() = default;

    virtual ~AudioBaseCodec() = default;

    virtual int32_t Init(const Media::Format &format) = 0;

    virtual int32_t ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer) = 0;

    virtual int32_t ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer) = 0;

    virtual int32_t Reset() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t Flush() = 0;

    virtual int32_t GetInputBufferSize() const = 0;

    virtual int32_t GetOutputBufferSize() const = 0;

    virtual Media::Format GetFormat() const noexcept = 0;

    virtual std::string_view GetCodecType() const noexcept = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif