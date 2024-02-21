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

#ifndef FFMPEG_CONVERTER_H
#define FFMPEG_CONVERTER_H

#include <string_view>
#include <string>
#include "meta/meta_key.h"
#include "meta/media_types.h"
#include "meta/mime_type.h"
#include "meta/video_types.h"
#include "meta/audio_types.h"
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
namespace OHOS {
namespace Media {
namespace Plugins {
class FFMpegConverter {
public:
    static ColorPrimary ConvertFFMpegToOHColorPrimaries(AVColorPrimaries ffColorPrimaries);
    static TransferCharacteristic ConvertFFMpegToOHColorTrans(AVColorTransferCharacteristic ffColorTrans);
    static MatrixCoefficient ConvertFFMpegToOHColorMatrix(AVColorSpace ffColorSpace);
    static int ConvertFFMpegToOHColorRange(AVColorRange ffColorRange);
    static ChromaLocation ConvertFFMpegToOHChromaLocation(AVChromaLocation ffChromaLocation);
    static HEVCProfile ConvertFFMpegToOHHEVCProfile(int ffHEVCProfile);
    static HEVCLevel ConvertFFMpegToOHHEVCLevel(int ffHEVCLevel);
    static AudioSampleFormat ConvertFFMpegAVCodecIdToOHAudioFormat(AVCodecID codecId);
    static AudioSampleFormat ConvertFFMpegToOHAudioFormat(AVSampleFormat ffSampleFormat);
    static AVSampleFormat ConvertOHAudioFormatToFFMpeg(AudioSampleFormat sampleFormat);
    static AudioChannelLayout ConvertFFToOHAudioChannelLayout(uint64_t ffChannelLayout);
    static AudioChannelLayout GetDefaultChannelLayout(int channels);
    static AudioChannelLayout ConvertFFToOHAudioChannelLayoutV2(uint64_t ffChannelLayout, int channels);
    static uint64_t ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout channelLayout);
    static std::string_view ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout);
    static int64_t ConvertAudioPtsToUs(int64_t pts, AVRational base);
    static std::string AVStrError(int errnum);

private:
    FFMpegConverter() = delete;
    ~FFMpegConverter() = delete;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif