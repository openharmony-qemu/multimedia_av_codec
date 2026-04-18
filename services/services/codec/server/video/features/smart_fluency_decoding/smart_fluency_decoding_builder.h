/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef SMART_FLUENCY_DECODING_BUILDER_H
#define SMART_FLUENCY_DECODING_BUILDER_H

#include <memory>
#include <optional>
#include <string>
#include "meta/format.h"
#include "smart_fluency_decoding_types.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
namespace SFD {
using AVBuffer = OHOS::Media::AVBuffer;

class SmartFluencyDecoding;

class SmartFluencyDecodingBuilder {
public:
    SmartFluencyDecodingBuilder();
    ~SmartFluencyDecodingBuilder();

    SmartFluencyDecodingBuilder& SetVideoWidth(int32_t width);
    SmartFluencyDecodingBuilder& SetVideoHeight(int32_t height);
    SmartFluencyDecodingBuilder& SetCodecMime(const std::string& mime);
    SmartFluencyDecodingBuilder& UpdatePlaybackSpeed(double speed);
    SmartFluencyDecodingBuilder& SetFromFormat(const Format& format);
    std::unique_ptr<SmartFluencyDecoding> Build();

private:
    static SFDCodecType ParseCodecType(const std::string& mime);
    bool ValidateParams();
    SFDConfig config_{};
    std::optional<double> playbackSpeed_{std::nullopt};
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SMART_FLUENCY_DECODING_BUILDER_H
