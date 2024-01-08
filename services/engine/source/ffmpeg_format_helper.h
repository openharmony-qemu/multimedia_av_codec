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

#ifndef MEDIA_AVCODEC_FFMPEG_FORMAT_HELPER
#define MEDIA_AVCODEC_FFMPEG_FORMAT_HELPER

#include <cstdint>
#include "meta/format.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/parseutils.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
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
    static void ParseMediaInfo(const AVFormatContext &avFormatContext, Media::Format &format);
    static void ParseTrackInfo(const AVStream &avStream, Media::Format &format);
    static void ParseHevcInfo(const AVFormatContext &avFormatContext, HevcParseFormat parse, Media::Format &format);

private:
    FFmpegFormatHelper() = delete;
    ~FFmpegFormatHelper() = delete;

    static void ParseBaseTrackInfo(const AVStream &avStream, Media::Format &format);
    static void ParseAVTrackInfo(const AVStream& avStream, Media::Format &format);
    static void ParseVideoTrackInfo(const AVStream& avStream, Media::Format &format);
    static void ParseAudioTrackInfo(const AVStream& avStream, Media::Format &format);
    static void ParseImageTrackInfo(const AVStream& avStream, Media::Format &format);
    static void ParseHvccBoxInfo(const AVStream& avStream, Media::Format &format);
    static void ParseColorBoxInfo(const AVStream& avStream, Media::Format &format);

    static void ParseInfoFromMetadata(const AVDictionary* metadata, const std::string_view key, Media::Format &format);
    static void PutInfoToFormat(const std::string_view &key, int32_t value, Media::Format& format);
    static void PutInfoToFormat(const std::string_view &key, int64_t value, Media::Format& format);
    static void PutInfoToFormat(const std::string_view &key, float value, Media::Format& format);
    static void PutInfoToFormat(const std::string_view &key, double value, Media::Format& format);
    static void PutInfoToFormat(const std::string_view &key, const std::string_view &value, Media::Format& format);
    static void PutBufferToFormat(const std::string_view &key, const uint8_t *addr, size_t size, Media::Format &format);
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_FFMPEG_FORMAT_HELPER