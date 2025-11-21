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

#ifndef MEDIA_AVCODEC_MIME_TYPE_H
#define MEDIA_AVCODEC_MIME_TYPE_H
#include <string_view>
#include <string>
#include <unordered_set>

namespace OHOS {
namespace MediaAVCodec {
/**
 * @enum Media mime type
 *
 * @since 4.0
 */
class AVCodecMimeType {
public:
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_FLAC = "audio/flac";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_VORBIS = "audio/vorbis";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AMRNB = "audio/3gpp";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_OPUS = "audio/opus";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_MPEG = "audio/mpeg";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AMRWB = "audio/amr-wb";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_VIVID = "audio/av3a";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_G711MU = "audio/g711mu";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_G711A = "audio/g711a";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_L2HC = "audio/l2hc";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_APE = "audio/x-ape";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_LBVC = "audio/lbvc";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_COOK = "audio/cook";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_EAC3 = "audio/eac3";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ALAC = "audio/alac";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_RAW = "audio/raw";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_GSM_MS = "audio/gsm_ms";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_GSM = "audio/gsm";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_WMAV1 = "audio/wmav1";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_WMAV2 = "audio/wmav2";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_WMAPRO = "audio/wmapro";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_MS = "audio/adpcm_ms";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_QT = "audio/adpcm_ima_qt";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_WAV = "audio/adpcm_ima_wav";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_DK3 = "audio/adpcm_ima_dk3";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_DK4 = "audio/adpcm_ima_dk4";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_WS = "audio/adpcm_ima_ws";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_SMJPEG = "audio/adpcm_ima_smjpeg";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_DAT4 = "audio/adpcm_ima_dat4";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_MTAF = "audio/adpcm_mtaf";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_ADX = "audio/adpcm_adx";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_AFC = "audio/adpcm_afc";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_AICA = "audio/adpcm_aica";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_CT = "audio/adpcm_ct";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_DTK = "audio/adpcm_dtk";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_G722 = "audio/adpcm_g722";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_G726 = "audio/adpcm_g726";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_G726LE = "audio/adpcm_g726le";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_AMV = "audio/adpcm_ima_amv";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_APC = "audio/adpcm_ima_apc";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_ISS = "audio/adpcm_ima_iss";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_OKI = "audio/adpcm_ima_oki";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_IMA_RAD = "audio/adpcm_ima_rad";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_PSX = "audio/adpcm_psx";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_SBPRO_2 = "audio/adpcm_sbpro_2";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_SBPRO_3 = "audio/adpcm_sbpro_3";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_SBPRO_4 = "audio/adpcm_sbpro_4";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_THP = "audio/adpcm_thp";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_THP_LE = "audio/adpcm_thp_le";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_XA = "audio/adpcm_xa";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ADPCM_YAMAHA = "audio/adpcm_yamaha";
    static constexpr std::string_view MEDIA_MIMETYPE_AUDIO_ILBC = "audio/ilbc";

    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_MSVIDEO1 = "video/msvideo1";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_VC1 = "video/vc1";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_RV30 = "video/rv30";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_RV40 = "video/rv40";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_MJPEG = "video/mjpeg";
    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_WMV3 = "video/wmv3";

    static constexpr std::string_view MEDIA_MIMETYPE_IMAGE_JPG = "image/jpeg";
    static constexpr std::string_view MEDIA_MIMETYPE_IMAGE_PNG = "image/png";
    static constexpr std::string_view MEDIA_MIMETYPE_IMAGE_BMP = "image/bmp";

    static constexpr std::string_view MEDIA_MIMETYPE_VIDEO_VVC = "video/vvc";

    static const std::unordered_set<std::string_view> &GetAudioCodecOuterSupportTable(bool isEncoder)
    {
        static const std::unordered_set<std::string_view> ENCODE_OUTER_SUPPORT_TABLE = {
            MEDIA_MIMETYPE_AUDIO_AAC,
            MEDIA_MIMETYPE_AUDIO_FLAC,
            MEDIA_MIMETYPE_AUDIO_G711MU,
            MEDIA_MIMETYPE_AUDIO_MPEG,
#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
            MEDIA_MIMETYPE_AUDIO_OPUS,
            MEDIA_MIMETYPE_AUDIO_AMRNB,
            MEDIA_MIMETYPE_AUDIO_AMRWB
#endif
        };
        static const std::unordered_set<std::string_view> DECODE_OUTER_SUPPORT_TABLE = {
            MEDIA_MIMETYPE_AUDIO_AAC,
            MEDIA_MIMETYPE_AUDIO_FLAC,
            MEDIA_MIMETYPE_AUDIO_G711MU,
            MEDIA_MIMETYPE_AUDIO_G711A,
            MEDIA_MIMETYPE_AUDIO_RAW,
            MEDIA_MIMETYPE_AUDIO_VORBIS,
            MEDIA_MIMETYPE_AUDIO_MPEG,
            MEDIA_MIMETYPE_AUDIO_AMRNB,
            MEDIA_MIMETYPE_AUDIO_AMRWB,
            MEDIA_MIMETYPE_AUDIO_APE,
            MEDIA_MIMETYPE_AUDIO_ALAC,
            MEDIA_MIMETYPE_AUDIO_GSM_MS,
            MEDIA_MIMETYPE_AUDIO_GSM,
            MEDIA_MIMETYPE_AUDIO_AC3,
            MEDIA_MIMETYPE_AUDIO_WMAV1,
            MEDIA_MIMETYPE_AUDIO_WMAV2,
            MEDIA_MIMETYPE_AUDIO_WMAPRO,
            MEDIA_MIMETYPE_AUDIO_ILBC,
#ifdef SUPPORT_CODEC_EAC3
            MEDIA_MIMETYPE_AUDIO_EAC3,
#endif
#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
            MEDIA_MIMETYPE_AUDIO_OPUS,
            MEDIA_MIMETYPE_AUDIO_VIVID
#endif
        };
        return isEncoder ? ENCODE_OUTER_SUPPORT_TABLE : DECODE_OUTER_SUPPORT_TABLE;
    }

    static bool CheckAudioCodecMimeSupportOuter(const std::string &mime, bool isEncoder)
    {
        return GetAudioCodecOuterSupportTable(isEncoder).count(mime);
    }

private:
    AVCodecMimeType() = delete;
    ~AVCodecMimeType() = delete;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_MIME_TYPE_H