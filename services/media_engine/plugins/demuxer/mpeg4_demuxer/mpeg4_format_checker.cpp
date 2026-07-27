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
#include "mpeg4_format_checker.h"
#include "mpeg4_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

using FourccSet = std::unordered_set<int32_t>;

// file format
static const FourccSet g_supportedFileBrandsFourcc = {
    FourccType("isom"), FourccType("iso2"), FourccType("iso3"), FourccType("iso4"),
    FourccType("mp41"), FourccType("mp42"),
    FourccType("avc1"), FourccType("avc3"),
    FourccType("hvc1"), FourccType("hev1"),
    FourccType("vvc1"), FourccType("vvi1"),
    FourccType("M4A "), FourccType("m4a "),
};

static const std::vector<std::string> g_supportedFileBrandsPrefixs = {
    "isom", "iso2", "iso3", "iso4",
    "mp41", "mp42",
    "avc1", "avc3",
    "hvc1", "hev1",
    "vvc1", "vvi1",
    "M4A ", "m4a ",
};

static const FourccSet g_unsupportedFileBrandsFourcc = {
    FourccType("qt  "), FourccType("MSNV"), FourccType("wmf "),
    FourccType("3gp4"), FourccType("3gp5"), FourccType("3gp6"),
    FourccType("3gp7"), FourccType("3gp8"), FourccType("3gp9"),
    FourccType("3ge6"), FourccType("3ge7"), FourccType("3ge9"),
    FourccType("3gf9"), FourccType("3gg6"), FourccType("3gg9"),
    FourccType("3gh9"), FourccType("3gm9"), FourccType("3gmA"),
    FourccType("3gr6"), FourccType("3gr9"), FourccType("3gs6"),
    FourccType("3gs9"), FourccType("3gt8"), FourccType("3gt9"),
    FourccType("3gtv"),
    FourccType("3g2a"), FourccType("3g2b"), FourccType("3g2c"),
    FourccType("M4V "), FourccType("m4v "),
    FourccType("M4B "), FourccType("m4b "), FourccType("M4P "), FourccType("m4p "),
    FourccType("mjp2"), FourccType("mj2s"),
    FourccType("iso5"), FourccType("iso6"), FourccType("iso7"),
    FourccType("iso8"), FourccType("iso9"),
    FourccType("fmp4"), FourccType("dash"), FourccType("isml"),
    FourccType("ismv"), FourccType("isma"),
    FourccType("F4V "), FourccType("F4P "), FourccType("F4A "), FourccType("F4B "),
    FourccType("nvr1"),
    FourccType("odcf"), FourccType("opf2"), FourccType("piff"),
    FourccType("isc2"), FourccType("ccff"), FourccType("uvvu"),
    FourccType("v3mt"), FourccType("v3mp"), FourccType("v3nt"), FourccType("v3st"),
    FourccType("cmfc"), FourccType("cmf2"),
    FourccType("heic"), FourccType("heix"), FourccType("heim"), FourccType("heis"),
    FourccType("hevc"), FourccType("hevx"), FourccType("hevm"), FourccType("hevs"),
    FourccType("mif1"), FourccType("msf1"), FourccType("mif2"), FourccType("miaf"),
    FourccType("avif"), FourccType("avio"), FourccType("avis"),
    FourccType("jp2 "), FourccType("jpx "), FourccType("jxl "),
};

// codec format
static const FourccSet g_supportedVideoCodecsFourcc = {
    FourccType("avc1"), FourccType("avc3"),
    FourccType("hvc1"), FourccType("hev1"),
    FourccType("vvc1"), FourccType("vvi1"),
    FourccType("mjpeg"), FourccType("png "), FourccType("bmp "),
};

static const FourccSet g_supportedAudioCodecsFourcc = {
    FourccType("mp4a"), FourccType(".mp3"), FourccType(".mp2"), FourccType(".mp1"),
    FourccType("av3a"), FourccType("samr"), FourccType("sawb"),
    0x6D730055,
};

static const FourccSet g_supportedSubtitleCodecsFourcc = {
    FourccType("wvtt")
};

static const FourccSet g_supportedAuxlCodecsFourcc = {
    FourccType("avc1"), FourccType("avc3"),
    FourccType("hvc1"), FourccType("hev1"),
    FourccType("mp4a"), FourccType(".mp3"), FourccType("mebx"),
};

static const std::map<std::string, std::string> g_supportedAllCodecName = {
    {MimeType::VIDEO_AVC, "h264"},
    {MimeType::VIDEO_HEVC, "hevc"},
    {MimeType::VIDEO_VVC, "vvc"},
    {MimeType::AUDIO_MPEG, "mp3"},
    {MimeType::AUDIO_AAC, "aac"},
    {MimeType::AUDIO_AVS3DA, "av3a"},
    {MimeType::AUDIO_AMR_NB, "amr_nb"},
    {MimeType::AUDIO_AMR_WB, "amr_wb"},
    {MimeType::TEXT_WEBVTT, "webvtt"},
    {MimeType::TIMED_METADATA, "unknown_codec"},
    {MimeType::IMAGE_JPG, "mjpeg"},
    {MimeType::IMAGE_PNG, "png"},
    {MimeType::IMAGE_BMP, "bmp"},
};

static const std::map<int32_t, std::string> g_supportedMimeFourcc = {
    { FourccType("avc1"), MimeType::VIDEO_AVC },
    { FourccType("avc3"), MimeType::VIDEO_AVC },
    { FourccType("hvc1"), MimeType::VIDEO_HEVC },
    { FourccType("hev1"), MimeType::VIDEO_HEVC },
    { FourccType("vvc1"), MimeType::VIDEO_VVC },
    { FourccType("vvi1"), MimeType::VIDEO_VVC },
    { FourccType(".mp1"), MimeType::AUDIO_MPEG },
    { FourccType(".mp2"), MimeType::AUDIO_MPEG },
    { FourccType(".mp3"), MimeType::AUDIO_MPEG },
    { 0x6D730055,         MimeType::AUDIO_MPEG },
    { FourccType("mp4a"), MimeType::AUDIO_AAC },
    { FourccType("av3a"), MimeType::AUDIO_AVS3DA },
    { FourccType("samr"), MimeType::AUDIO_AMR_NB },
    { FourccType("sawb"), MimeType::AUDIO_AMR_WB },
    { FourccType("wvtt"), MimeType::TEXT_WEBVTT },
    { FourccType("mebx"), MimeType::TIMED_METADATA },
    { FourccType("mjpeg"), MimeType::IMAGE_JPG },
    { FourccType("png "), MimeType::IMAGE_PNG },
    { FourccType("bmp "), MimeType::IMAGE_BMP },
};

static const std::map<uint8_t, std::string> g_supportedObjectType = {
    // 视频编解码器
    {0x21, MimeType::VIDEO_AVC},      // H.264/AVC
    {0x23, MimeType::VIDEO_HEVC},     // H.265/HEVC
    {0x33, MimeType::VIDEO_VVC},      // H.266/VVC (OHOS扩展)
    // 音频编解码器
    {0x40, MimeType::AUDIO_AAC},      // MPEG-4 AAC
    {0x66, MimeType::AUDIO_AAC},      // MPEG-2 AAC Main
    {0x67, MimeType::AUDIO_AAC},      // MPEG-2 AAC Low Complexity
    {0x68, MimeType::AUDIO_AAC},      // MPEG-2 AAC SSR
    {0x69, MimeType::AUDIO_MPEG},     // MPEG-2 Layer 3 (MP3) / MPEG-1 Layer 2 (MP2)
    {0x6B, MimeType::AUDIO_MPEG},     // MPEG-1 Layer 3 (MP3)
};

static const std::vector<std::string_view> g_supportImageMimeType = {
    MimeType::IMAGE_JPG,
    MimeType::IMAGE_PNG,
    MimeType::IMAGE_BMP,
};

static const std::vector<std::string_view> g_supportVideoMimeType = {
    MimeType::VIDEO_AVC,
    MimeType::VIDEO_HEVC,
    MimeType::VIDEO_VVC,
};

static const std::vector<std::string_view> g_supportVideoMimeTypeTemp = {
    MimeType::VIDEO_MPEG4,
    MimeType::VIDEO_H263,
    MimeType::VIDEO_AVC,
    MimeType::VIDEO_HEVC,
    MimeType::VIDEO_VVC,
    MimeType::VIDEO_MPEG2,
};

static const std::vector<std::string_view> g_supportAudioMimeType = {
    MimeType::AUDIO_MPEG,
    MimeType::AUDIO_AAC,
    MimeType::AUDIO_AVS3DA,
    MimeType::AUDIO_AMR_NB,
    MimeType::AUDIO_AMR_WB,
};

static const std::vector<std::string_view> g_supportAudioMimeTypeTemp = {
    MimeType::AUDIO_FLAC,
    MimeType::AUDIO_MPEG,
    MimeType::AUDIO_AAC,
    MimeType::AUDIO_AVS3DA,
    MimeType::AUDIO_G711A,
    MimeType::AUDIO_G711MU,
    MimeType::AUDIO_RAW,
    MimeType::AUDIO_VORBIS,
    MimeType::AUDIO_OPUS,
    MimeType::AUDIO_AMR_NB,
    MimeType::AUDIO_AMR_WB,
    MimeType::AUDIO_ALAC,
    MimeType::AUDIO_DTS,
};

static const std::map<TrackType, MediaType> g_supportMediaType = {
    {VIDEO_TYPE, MediaType::VIDEO},
    {AUDIO_TYPE, MediaType::AUDIO},
    {SUBTITLE_TYPE, MediaType::SUBTITLE},
    {TIMEDMETA_TYPE, MediaType::TIMEDMETA},
    {AUXILIARY_TYPE, MediaType::AUXILIARY},
};

static const std::vector<TrackType> g_supportMediaInfoType = {
    TrackType::VIDEO_TYPE,
    TrackType::AUDIO_TYPE,
    TrackType::TIMEDMETA_TYPE,
    TrackType::AUXILIARY_TYPE,
};


bool IsSupportedBrand(int32_t brand)
{
    return g_supportedFileBrandsFourcc.count(brand) > 0;
}

bool IsSupportedBrandPrefix(const std::string& majorBrand)
{
    for (const auto& prefix : g_supportedFileBrandsPrefixs) {
        if (majorBrand.size() >= prefix.size() && majorBrand.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

bool IsSupportedBrand(const std::string& brand)
{
    uint32_t majorBrandInt = (static_cast<uint32_t>(brand[0]) << 24) |
                             (static_cast<uint32_t>(brand[1]) << 16) |
                             (static_cast<uint32_t>(brand[2]) << 8) |
                             static_cast<uint32_t>(brand[3]);
    return IsSupportedBrand(majorBrandInt) || IsSupportedBrandPrefix(brand);
}

bool IsUnsupportedBrand(int32_t brand)
{
    return g_unsupportedFileBrandsFourcc.count(brand) > 0;
}

bool IsSupportedVideoCodec(int32_t fourcc)
{
    return g_supportedVideoCodecsFourcc.count(fourcc) > 0;
}

bool IsSupportedAudioCodec(int32_t fourcc)
{
    return g_supportedAudioCodecsFourcc.count(fourcc) > 0;
}

bool IsSupportedSubtitleCodec(int32_t fourcc)
{
    return g_supportedSubtitleCodecsFourcc.count(fourcc) > 0;
}

bool IsSupportedAuxiliaryCodec(int32_t fourcc)
{
    return g_supportedAuxlCodecsFourcc.count(fourcc) > 0;
}

bool IsImageMimeType(const std::string& mime)
{
    return (std::find(g_supportImageMimeType.cbegin(), g_supportImageMimeType.cend(),
        mime) != g_supportImageMimeType.cend());
}

bool IsVideoMimeType(const std::string& mime)
{
    return (std::find(g_supportVideoMimeType.cbegin(), g_supportVideoMimeType.cend(),
        mime) != g_supportVideoMimeType.cend());
}

bool IsAudioMimeType(const std::string& mime)
{
    return (std::find(g_supportAudioMimeType.cbegin(), g_supportAudioMimeType.cend(),
        mime) != g_supportAudioMimeType.cend());
}

bool IsSupportCodec(int32_t fourcc, TrackType trackType, const std::string& mime)
{
    if (mime == MimeType::INVALID_TYPE) {
        return false;
    }
    switch (trackType) {
        case VIDEO_TYPE: {
            return (IsSupportedVideoCodec(fourcc) || IsImageMimeType(mime));
        }
        case AUDIO_TYPE: {
            return IsSupportedAudioCodec(fourcc);
        }
        case SUBTITLE_TYPE: {
            return IsSupportedSubtitleCodec(fourcc);
        }
        case TIMEDMETA_TYPE:
        case AUXILIARY_TYPE: {
            return IsSupportedAuxiliaryCodec(fourcc);
        }
        default: {
            return false;
        }
    }
}

bool NeedParseVideoInfo(std::string mimeType, TrackType trackType)
{
    return (trackType == VIDEO_TYPE  && !IsImageMimeType(mimeType)) ||
           (trackType == AUXILIARY_TYPE  && IsVideoMimeType(mimeType));
}

bool NeedParseAudioInfo(std::string mimeType, TrackType trackType)
{
    return (trackType == AUDIO_TYPE) ||
           (trackType == AUXILIARY_TYPE && IsAudioMimeType(mimeType));
}

bool NeedParseMediaTrackInfo(TrackType trackType)
{
    return (std::find(g_supportMediaInfoType.cbegin(), g_supportMediaInfoType.cend(),
        trackType) != g_supportMediaInfoType.cend());
}

std::string GetMimeType(int32_t atomtype)
{
    auto it = g_supportedMimeFourcc.find(atomtype);
    if (it != g_supportedMimeFourcc.end()) {
        return it->second;
    }
    return MimeType::INVALID_TYPE;
}

std::string GetMimeTypeByObjectType(uint8_t objectTypeId)
{
    auto it = g_supportedObjectType.find(objectTypeId);
    return (it != g_supportedObjectType.end()) ? it->second : "";
}

std::string GetCodecName(std::string mimeType)
{
    auto it = g_supportedAllCodecName.find(mimeType);
    return (it != g_supportedAllCodecName.end()) ? it->second : "";
}

MediaType GetMediaType(TrackType trackType)
{
    auto it = g_supportMediaType.find(trackType);
    return (it != g_supportMediaType.end()) ? it->second : MediaType::UNKNOWN;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS