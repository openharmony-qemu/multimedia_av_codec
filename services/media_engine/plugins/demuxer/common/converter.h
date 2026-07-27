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

#ifndef CONVERTER_H
#define CONVERTER_H

#include <string_view>
#include <string>
#include "meta/meta.h"
#include "meta/meta_key.h"
#include "meta/media_types.h"
#include "meta/mime_type.h"
#include "meta/video_types.h"
#include "meta/audio_types.h"
#include "stream_parser.h"
#ifdef USE_FF_DEMUXER
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"
#include "libavcodec/bsf.h"
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif
#endif
namespace OHOS {
namespace Media {
namespace Plugins {
struct HdrBoxInfo {
    bool haveHdrDoblyVisionBox = false;
    bool haveHdrVividBox = false;
};

class Converter {
public:
    static ColorPrimary ConvertFFMpegToOHColorPrimaries(int32_t colorPrimaries);
    static TransferCharacteristic ConvertFFMpegToOHColorTrans(int32_t colorTransfer);
    static MatrixCoefficient ConvertFFMpegToOHColorMatrix(int32_t colorSpace);
    static int32_t ConvertFFMpegToOHColorRange(int32_t colorRange);
    static ChromaLocation ConvertFFMpegToOHChromaLocation(int32_t chromaLocation);
    static HEVCProfile ConvertToOHHEVCProfile(int ffHEVCProfile);
    static HEVCLevel ConvertToOHHEVCLevel(int ffHEVCLevel);
#ifdef USE_FF_DEMUXER
    static AudioSampleFormat ConvertFFMpegAVCodecIdToOHAudioFormat(AVCodecID codecId);
    static AudioSampleFormat ConvertFFMpegToOHAudioFormat(AVSampleFormat ffSampleFormat);
    static AudioChannelLayout ConvertFFToOHAudioChannelLayoutV2(uint64_t ffChannelLayout, int channels);
    static AudioChannelLayout ConvertAudioVividToOHAudioChannelLayout(uint64_t ffChannelLayout, int channels);
#endif
    static void ParseColorBoxInfo(HevcParseFormat parse, Meta &format);
    static void ParseHdrTypeInfo(HdrBoxInfo hdrBoxInfo, Meta &format, HevcParseFormat parse);
    static std::string ToLower(const std::string& str);
    static bool IsUTF8(const std::string &data);
    static std::string ConvertGBKToUTF8(const std::string &strGbk);
    static bool IsGBK(const char* data);
    static AudioChannelLayout GetDefaultChannelLayout(int channels);

private:
    Converter() = delete;
    ~Converter() = delete;
    static std::string_view ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout);
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // CONVERTER_H