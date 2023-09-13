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
#include "format.h"

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
enum DataType : uint32_t {
    DATA_TYPE_INT32,  // Int32
    DATA_TYPE_INT64,  // Int64
    DATA_TYPE_FLOAT,  // Float
    DATA_TYPE_DOUBLE, // Double
    DATA_TYPE_STRING, // String
};

class FFmpegFormatHelper {
public:
    static void ParseMediaInfo(const AVFormatContext& avFormatContext, Format &format);
    static void ParseTrackInfo(const AVStream& avStream, Format &format);
private:
    FFmpegFormatHelper() = delete;
    ~FFmpegFormatHelper() = delete;

    static void ParseCommonTrackInfo(const AVStream& avStream, Format &format);
    static void ParseVideoTrackInfo(const AVStream& avStream, Format &format);
    static void ParseAudioTrackInfo(const AVStream& avStream, Format &format);

    static void ParseInfoFromMetadata(const AVDictionary* metadata, const std::string_view key, Format &format);
    // static std::shared_ptr<AVCodecContext> InitCodecContext(const AVStream& avStream);

    static void PutInfoToFormat(const std::string_view &key, int32_t value, Format& format);
    static void PutInfoToFormat(const std::string_view &key, int64_t value, Format& format);
    static void PutInfoToFormat(const std::string_view &key, float value, Format& format);
    static void PutInfoToFormat(const std::string_view &key, double value, Format& format);
    static void PutInfoToFormat(const std::string_view &key, const std::string_view &value, Format& format);
    static void PutBufferToFormat(const std::string_view &key, const uint8_t *dataAddr, size_t size, Format &format);
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_FFMPEG_FORMAT_HELPER