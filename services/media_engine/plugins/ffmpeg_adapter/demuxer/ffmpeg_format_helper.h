/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef FFMPEG_FORMAT_HELPER_H
#define FFMPEG_FORMAT_HELPER_H

#include <cstdint>
#include "meta/meta.h"
#include "ffmpeg_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/parseutils.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
struct HevcParseFormat {
    int32_t isHdrVivid = 0;
    int32_t colorRange = 0;
    uint8_t colorPrimaries = 0x02;
    uint8_t colorTransfer = 0x02;
    uint8_t colorMatrixCoeff = 0x02;
    uint8_t profile = 0;
    uint8_t level = 0;
    uint32_t chromaLocation = 0;
    uint32_t picWidInLumaSamples = 0;
    uint32_t picHetInLumaSamples = 0;
};

class FFmpegFormatHelper {
public:
    static void ParseMediaInfo(const AVFormatContext& avFormatContext, Meta& format);
    static void ParseTrackInfo(const AVStream& avStream, Meta& format);
    static void ParseUserMeta(const AVFormatContext& avFormatContext, std::shared_ptr<Meta> format);
    static void ParseHevcInfo(const AVFormatContext& avFormatContext, HevcParseFormat parse, Meta &format);
    static FileType GetFileTypeByName(const AVFormatContext& avFormatContext);
private:
    FFmpegFormatHelper() = delete;
    ~FFmpegFormatHelper() = delete;

    static void ParseBaseTrackInfo(const AVStream& avStream, Meta &format);
    static void ParseAVTrackInfo(const AVStream& avStream, Meta &format);
    static void ParseVideoTrackInfo(const AVStream& avStream, Meta &format);
    static void ParseAudioTrackInfo(const AVStream& avStream, Meta &format);
    static void ParseImageTrackInfo(const AVStream& avStream, Meta &format);
    static void ParseHvccBoxInfo(const AVStream& avStream, Meta &format);
    static void ParseColorBoxInfo(const AVStream& avStream, Meta &format);

    static void ParseLocationInfo(const AVFormatContext& avFormatContext, Meta &format);

    static void ParseInfoFromMetadata(const AVDictionary* metadata, const TagType key, Meta &format);
    static void PutInfoToFormat(const Tag key, int32_t value, Meta &format);
    static void PutInfoToFormat(const Tag key, int64_t value, Meta &format);
    static void PutInfoToFormat(const Tag key, float value, Meta &format);
    static void PutInfoToFormat(const Tag key, double value, Meta &format);
    static void PutInfoToFormat(const Tag key, const std::string_view &value, Meta &format);
    static void PutBufferToFormat(const Tag key, const uint8_t *addr, size_t size, Meta &format);
    static void ParseRotationFromMatrix(const AVStream& avStream, Meta &format);
};
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // FFMPEG_FORMAT_HELPER_H