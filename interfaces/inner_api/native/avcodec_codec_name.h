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

#ifndef AVCODEC_CODEC_KEY_H
#define AVCODEC_CODEC_KEY_H
#include <string_view>

namespace OHOS {
namespace MediaAVCodec {
class AVCodecCodecName {
public:
    static constexpr std::string_view AUDIO_DECODER_MP3_NAME = "OH.Media.Codec.Decoder.Audio.Mpeg";
    static constexpr std::string_view AUDIO_DECODER_AAC_NAME = "OH.Media.Codec.Decoder.Audio.AAC";
    static constexpr std::string_view AUDIO_DECODER_API9_AAC_NAME = "avdec_aac";
    static constexpr std::string_view AUDIO_DECODER_VORBIS_NAME = "OH.Media.Codec.Decoder.Audio.Vorbis";
    static constexpr std::string_view AUDIO_DECODER_FLAC_NAME = "OH.Media.Codec.Decoder.Audio.Flac";

    static constexpr std::string_view AUDIO_ENCODER_FLAC_NAME = "OH.Media.Codec.Encoder.Audio.Flac";
    static constexpr std::string_view AUDIO_ENCODER_AAC_NAME = "OH.Media.Codec.Encoder.Audio.AAC";
    static constexpr std::string_view AUDIO_ENCODER_API9_AAC_NAME = "avenc_aac";

    static constexpr std::string_view VIDEO_DECODER_AVC_NAME = "OH.Media.Codec.Decoder.Video.AVC";

private:
    AVCodecCodecName() = delete;
    ~AVCodecCodecName() = delete;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif