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

#ifndef MEDIA_AVCODEC_CODEC_KEY_H
#define MEDIA_AVCODEC_CODEC_KEY_H
#include <string_view>
#include <string>
#include <unordered_set>

namespace OHOS {
namespace MediaAVCodec {
class AVCodecCodecName {
public:
    static constexpr std::string_view AUDIO_DECODER_MP3_NAME = "OH.Media.Codec.Decoder.Audio.Mpeg";
    static constexpr std::string_view AUDIO_DECODER_AAC_NAME = "OH.Media.Codec.Decoder.Audio.AAC";
    static constexpr std::string_view AUDIO_DECODER_OPUS_NAME = "OH.Media.Codec.Decoder.Audio.Opus";
    static constexpr std::string_view AUDIO_DECODER_API9_AAC_NAME = "avdec_aac";
    static constexpr std::string_view AUDIO_DECODER_VORBIS_NAME = "OH.Media.Codec.Decoder.Audio.Vorbis";
    static constexpr std::string_view AUDIO_DECODER_FLAC_NAME = "OH.Media.Codec.Decoder.Audio.Flac";
    static constexpr std::string_view AUDIO_DECODER_AMRNB_NAME = "OH.Media.Codec.Decoder.Audio.Amrnb";
    static constexpr std::string_view AUDIO_DECODER_AMRWB_NAME = "OH.Media.Codec.Decoder.Audio.Amrwb";
    static constexpr std::string_view AUDIO_DECODER_VIVID_NAME = "OH.Media.Codec.Decoder.Audio.Vivid";
    static constexpr std::string_view AUDIO_DECODER_G711MU_NAME = "OH.Media.Codec.Decoder.Audio.G711mu";
    static constexpr std::string_view AUDIO_DECODER_G711A_NAME = "OH.Media.Codec.Decoder.Audio.G711a";
    static constexpr std::string_view AUDIO_DECODER_APE_NAME = "OH.Media.Codec.Decoder.Audio.Ape";
    static constexpr std::string_view AUDIO_DECODER_L2HC_NAME = "OH.Media.Codec.Decoder.Audio.L2HC";
    static constexpr std::string_view AUDIO_DECODER_LBVC_NAME = "OH.Media.Codec.Decoder.Audio.LBVC";
    static constexpr std::string_view AUDIO_DECODER_COOK_NAME = "OH.Media.Codec.Decoder.Audio.COOK";
    static constexpr std::string_view AUDIO_DECODER_AC3_NAME = "OH.Media.Codec.Decoder.Audio.AC3";
    static constexpr std::string_view AUDIO_DECODER_EAC3_NAME = "OH.Media.Codec.Decoder.Audio.EAC3";
    static constexpr std::string_view AUDIO_DECODER_RAW_NAME = "OH.Media.Codec.Decoder.Audio.Raw";

    static constexpr std::string_view AUDIO_ENCODER_FLAC_NAME = "OH.Media.Codec.Encoder.Audio.Flac";
    static constexpr std::string_view AUDIO_ENCODER_OPUS_NAME = "OH.Media.Codec.Encoder.Audio.Opus";
    static constexpr std::string_view AUDIO_ENCODER_G711MU_NAME = "OH.Media.Codec.Encoder.Audio.G711mu";
    static constexpr std::string_view AUDIO_ENCODER_AAC_NAME = "OH.Media.Codec.Encoder.Audio.AAC";
    static constexpr std::string_view AUDIO_ENCODER_VENDOR_AAC_NAME = "OH.Media.Codec.Encoder.Audio.Vendor.AAC";
    static constexpr std::string_view AUDIO_ENCODER_L2HC_NAME = "OH.Media.Codec.Encoder.Audio.L2HC";
    static constexpr std::string_view AUDIO_ENCODER_LBVC_NAME = "OH.Media.Codec.Encoder.Audio.LBVC";
    static constexpr std::string_view AUDIO_ENCODER_AMRNB_NAME = "OH.Media.Codec.Encoder.Audio.Amrnb";
    static constexpr std::string_view AUDIO_ENCODER_AMRWB_NAME = "OH.Media.Codec.Encoder.Audio.Amrwb";
    static constexpr std::string_view AUDIO_ENCODER_API9_AAC_NAME = "avenc_aac";
    static constexpr std::string_view AUDIO_ENCODER_MP3_NAME = "OH.Media.Codec.Encoder.Audio.Mp3";

    static constexpr std::string_view VIDEO_ENCODER_AVC_NAME = "OH.Media.Codec.Encoder.Video.AVC";
    static constexpr std::string_view VIDEO_DECODER_AVC_NAME = "OH.Media.Codec.Decoder.Video.AVC";
    static constexpr std::string_view VIDEO_DECODER_HEVC_NAME = "OH.Media.Codec.Decoder.Video.HEVC";
    static constexpr std::string_view VIDEO_DECODER_MPEG2_NAME = "OH.Media.Codec.Decoder.Video.MPEG2";
    static constexpr std::string_view VIDEO_DECODER_MPEG4_NAME = "OH.Media.Codec.Decoder.Video.MPEG4";
    static constexpr std::string_view VIDEO_DECODER_H263_NAME = "OH.Media.Codec.Decoder.Video.H263";
    static constexpr std::string_view VIDEO_DECODER_VVC_NAME = "OH.Media.Codec.Decoder.Video.VVC";
    static constexpr std::string_view VIDEO_DECODER_RV30_NAME = "OH.Media.Codec.Decoder.Video.Rv30";
    static constexpr std::string_view VIDEO_DECODER_RV40_NAME = "OH.Media.Codec.Decoder.Video.Rv40";
    static constexpr std::string_view VIDEO_DECODER_MJPEG_NAME = "OH.Media.Codec.Decoder.Video.MJPEG";

    static const std::unordered_set<std::string_view> &GetAudioCodecOuterSupportTable()
    {
        static const std::unordered_set<std::string_view> OUTER_SUPPORT_TABLE = {
            AUDIO_DECODER_AAC_NAME,
            AUDIO_ENCODER_AAC_NAME,
            AUDIO_DECODER_FLAC_NAME,
            AUDIO_ENCODER_FLAC_NAME,
            AUDIO_DECODER_VORBIS_NAME,
            AUDIO_DECODER_AMRNB_NAME,
            AUDIO_DECODER_MP3_NAME,
            AUDIO_ENCODER_MP3_NAME,
            AUDIO_DECODER_AMRWB_NAME,
            AUDIO_DECODER_G711MU_NAME,
            AUDIO_ENCODER_G711MU_NAME,
            AUDIO_DECODER_G711A_NAME,
            AUDIO_DECODER_APE_NAME,
            AUDIO_DECODER_RAW_NAME,
            AUDIO_DECODER_AC3_NAME,
#ifdef SUPPORT_CODEC_EAC3
            AUDIO_DECODER_EAC3_NAME,
#endif
#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
            AUDIO_ENCODER_VENDOR_AAC_NAME,
            AUDIO_DECODER_OPUS_NAME,
            AUDIO_ENCODER_OPUS_NAME,
            AUDIO_DECODER_VIVID_NAME,
            AUDIO_ENCODER_AMRNB_NAME,
            AUDIO_ENCODER_AMRWB_NAME
#endif
        };
        return OUTER_SUPPORT_TABLE;
    }

    static bool CheckAudioCodecNameSupportOuter(const std::string &name)
    {
        return GetAudioCodecOuterSupportTable().count(name);
    }

private:
    AVCodecCodecName() = delete;
    ~AVCodecCodecName() = delete;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_CODEC_KEY_H
