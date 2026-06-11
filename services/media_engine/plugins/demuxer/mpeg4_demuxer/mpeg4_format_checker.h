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

#ifndef MPEG4_FORMAT_CHECKER_H
#define MPEG4_FORMAT_CHECKER_H

#include <cstdint>
#include <string>
#include <unordered_set>
#include "meta/media_types.h"
#include "mpeg4_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

bool IsSupportedBrand(int32_t brand);
bool IsSupportedBrand(const std::string& brand);
bool IsUnsupportedBrand(int32_t brand);
bool IsSupportedVideoCodec(int32_t fourcc);
bool IsSupportedAudioCodec(int32_t fourcc);
bool IsSupportedSubtitleCodec(int32_t fourcc);
bool IsSupportedAuxiliaryCodec(int32_t fourcc);
bool IsVideoMimeType(const std::string& mime);
bool IsAudioMimeType(const std::string& mime);
bool IsImageMimeType(const std::string& mime);
bool IsSupportCodec(int32_t fourcc, TrackType trackType, const std::string& mime);

bool NeedParseVideoInfo(std::string mimeType, TrackType trackType);
bool NeedParseAudioInfo(std::string mimeType, TrackType trackType);
bool NeedParseMediaTrackInfo(TrackType trackType);

std::string GetMimeType(int32_t atomtype);
std::string GetMimeTypeByObjectType(uint8_t objectTypeId);
std::string GetCodecName(std::string mimeType);
MediaType GetMediaType(TrackType trackType);
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif