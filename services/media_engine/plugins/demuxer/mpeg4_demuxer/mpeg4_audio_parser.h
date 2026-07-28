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

#ifndef MPEG4_AUDIO_PARSER_H
#define MPEG4_AUDIO_PARSER_H
#include "plugin/plugin_definition.h"
#include "plugin/plugin_info.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
class MPEG4AudioParser {
public:
    explicit MPEG4AudioParser();
    ~MPEG4AudioParser();
    AudioChannelLayout FindValidChannelLayout(uint64_t layoutMask);
    AudioChannelLayout GetDefaultChannelLayout(int32_t channels);
    Status ParseAudioFrame(uint8_t* data, uint32_t size, std::string mime, uint32_t trackIndex, MediaInfo& mediaInfo);

private:
    uint8_t* audioData_ = nullptr;
    uint32_t dataSize_ = 0;
    uint32_t index_ = 0;
    Status ParseMpegAudio(MediaInfo& mediaInfo);
    Status ParseAacAudio(MediaInfo& mediaInfo);
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_AUDIO_PARSER_H
