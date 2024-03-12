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

#ifndef CODECLIST_CORE_MOCK_H
#define CODECLIST_CORE_MOCK_H

#include <gmock/gmock.h>
#include <mutex>
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "meta/format.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
enum class CodecType : int32_t {
    AVCODEC_INVALID = -1,
    AVCODEC_HCODEC = 0,
    AVCODEC_VIDEO_CODEC,
    AVCODEC_AUDIO_CODEC,
};
class CodecListCore;
class CodecListCoreMock {
public:
    CodecListCoreMock() = default;
    ~CodecListCoreMock() = default;

    MOCK_METHOD(void, CodecListCoreCtor, ());
    MOCK_METHOD(void, CodecListCoreDtor, ());

    MOCK_METHOD(std::string, FindEncoder, (const Media::Format &format));
    MOCK_METHOD(std::string, FindDecoder, (const Media::Format &format));
    MOCK_METHOD(CodecType, FindCodecType, (std::string codecName));
    MOCK_METHOD(int32_t, GetCapability,
                (CapabilityData & capData, const std::string &mime, const bool isEncoder,
                 const AVCodecCategory &category));
};

class CodecListCore {
public:
    static void RegisterMock(std::shared_ptr<CodecListCoreMock> &mock);

    CodecListCore();
    ~CodecListCore();
    std::string FindEncoder(const Media::Format &format);
    std::string FindDecoder(const Media::Format &format);
    CodecType FindCodecType(std::string codecName);
    int32_t GetCapability(CapabilityData &capData, const std::string &mime, const bool isEncoder,
                          const AVCodecCategory &category);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_CORE_MOCK_H
