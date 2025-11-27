/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "FfmpegFormatHelper"

#include <algorithm>
#include <charconv>
#include <regex>
#include <iconv.h>
#include <sstream>
#include "ffmpeg_converter.h"
#include "meta/meta_key.h"
#include "meta/media_types.h"
#include "meta/mime_type.h"
#include "meta/video_types.h"
#include "meta/audio_types.h"
#include "common/log.h"
#include "ffmpeg_format_helper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avutil.h"
#include "libavutil/display.h"
#ifdef __cplusplus
}
#endif

#define DISPLAY_MATRIX_SIZE (static_cast<size_t>(9))
#define CONVERT_MATRIX_SIZE (static_cast<size_t>(4))
#define CUVA_VERSION_MAP (static_cast<uint16_t>(1))
#define TERMINAL_PROVIDE_CODE (static_cast<uint16_t>(4))
#define TERMINAL_PROVIDE_ORIENTED_CODE (static_cast<uint16_t>(5))

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegFormatHelper" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const uint32_t DOUBLE_BYTES = 2;
const uint32_t KEY_PREFIX_LEN = 20;
const uint32_t VALUE_PREFIX_LEN = 8;
const uint32_t VALID_LOCATION_LEN = 2;
const int32_t VIDEO_ROTATION_360 = 360;
const int32_t AV3A_SAMPLE_8BITS = 8;
const int32_t AV3A_SAMPLE_16BITS = 16;
const int32_t AV3A_SAMPLE_24BITS = 24;

const unsigned char MASK_80 = 0x80;
const unsigned char MASK_C0 = 0xC0;
const int32_t MIN_BYTES = 2;
const int32_t MAX_BYTES = 6;

const int32_t INVALID_VAL = -1;
const int32_t POS_1 = 1;
const int32_t VALUE_2 = 2;
const int32_t VALUE_4 = 4;
const int32_t VALUE_10 = 10;

const size_t BRAND_CODE_LEN = 4; // 4 is brand code length

static std::map<AVMediaType, MediaType> g_convertFfmpegTrackType = {
    {AVMEDIA_TYPE_VIDEO, MediaType::VIDEO},
    {AVMEDIA_TYPE_AUDIO, MediaType::AUDIO},
    {AVMEDIA_TYPE_SUBTITLE, MediaType::SUBTITLE},
    {AVMEDIA_TYPE_TIMEDMETA, MediaType::TIMEDMETA},
    {AVMEDIA_TYPE_AUXILIARY, MediaType::AUXILIARY}
};

static std::map<AVCodecID, std::string_view> g_codecIdToMime = {
    {AV_CODEC_ID_MP1, MimeType::AUDIO_MPEG},
    {AV_CODEC_ID_MP2, MimeType::AUDIO_MPEG},
    {AV_CODEC_ID_MP3, MimeType::AUDIO_MPEG},
    {AV_CODEC_ID_FLAC, MimeType::AUDIO_FLAC},
    {AV_CODEC_ID_AAC, MimeType::AUDIO_AAC},
    {AV_CODEC_ID_AAC_LATM, MimeType::AUDIO_AAC},
    {AV_CODEC_ID_VORBIS, MimeType::AUDIO_VORBIS},
    {AV_CODEC_ID_OPUS, MimeType::AUDIO_OPUS},
    {AV_CODEC_ID_AMR_NB, MimeType::AUDIO_AMR_NB},
    {AV_CODEC_ID_AMR_WB, MimeType::AUDIO_AMR_WB},
    {AV_CODEC_ID_ADPCM_MS, MimeType::AUDIO_ADPCM_MS},
    {AV_CODEC_ID_ADPCM_IMA_QT, MimeType::AUDIO_ADPCM_IMA_QT},
    {AV_CODEC_ID_ADPCM_IMA_WAV, MimeType::AUDIO_ADPCM_IMA_WAV},
    {AV_CODEC_ID_ADPCM_IMA_DK3, MimeType::AUDIO_ADPCM_IMA_DK3},
    {AV_CODEC_ID_ADPCM_IMA_DK4, MimeType::AUDIO_ADPCM_IMA_DK4},
    {AV_CODEC_ID_ADPCM_IMA_WS, MimeType::AUDIO_ADPCM_IMA_WS},
    {AV_CODEC_ID_ADPCM_IMA_SMJPEG, MimeType::AUDIO_ADPCM_IMA_SMJPEG},
    {AV_CODEC_ID_ADPCM_IMA_DAT4, MimeType::AUDIO_ADPCM_IMA_DAT4},
    {AV_CODEC_ID_ADPCM_MTAF, MimeType::AUDIO_ADPCM_MTAF},
    {AV_CODEC_ID_ADPCM_ADX, MimeType::AUDIO_ADPCM_ADX},
    {AV_CODEC_ID_ADPCM_AFC, MimeType::AUDIO_ADPCM_AFC},
    {AV_CODEC_ID_ADPCM_AICA, MimeType::AUDIO_ADPCM_AICA},
    {AV_CODEC_ID_ADPCM_CT, MimeType::AUDIO_ADPCM_CT},
    {AV_CODEC_ID_ADPCM_DTK, MimeType::AUDIO_ADPCM_DTK},
    {AV_CODEC_ID_ADPCM_G722, MimeType::AUDIO_ADPCM_G722},
    {AV_CODEC_ID_ADPCM_G726, MimeType::AUDIO_ADPCM_G726},
    {AV_CODEC_ID_ADPCM_G726LE, MimeType::AUDIO_ADPCM_G726LE},
    {AV_CODEC_ID_ADPCM_IMA_AMV, MimeType::AUDIO_ADPCM_IMA_AMV},
    {AV_CODEC_ID_ADPCM_IMA_APC, MimeType::AUDIO_ADPCM_IMA_APC},
    {AV_CODEC_ID_ADPCM_IMA_ISS, MimeType::AUDIO_ADPCM_IMA_ISS},
    {AV_CODEC_ID_ADPCM_IMA_OKI, MimeType::AUDIO_ADPCM_IMA_OKI},
    {AV_CODEC_ID_ADPCM_IMA_RAD, MimeType::AUDIO_ADPCM_IMA_RAD},
    {AV_CODEC_ID_ADPCM_PSX, MimeType::AUDIO_ADPCM_PSX},
    {AV_CODEC_ID_ADPCM_SBPRO_2, MimeType::AUDIO_ADPCM_SBPRO_2},
    {AV_CODEC_ID_ADPCM_SBPRO_3, MimeType::AUDIO_ADPCM_SBPRO_3},
    {AV_CODEC_ID_ADPCM_SBPRO_4, MimeType::AUDIO_ADPCM_SBPRO_4},
    {AV_CODEC_ID_ADPCM_THP, MimeType::AUDIO_ADPCM_THP},
    {AV_CODEC_ID_ADPCM_THP_LE, MimeType::AUDIO_ADPCM_THP_LE},
    {AV_CODEC_ID_ADPCM_XA, MimeType::AUDIO_ADPCM_XA},
    {AV_CODEC_ID_ADPCM_YAMAHA, MimeType::AUDIO_ADPCM_YAMAHA},
    {AV_CODEC_ID_H264, MimeType::VIDEO_AVC},
    {AV_CODEC_ID_MPEG4, MimeType::VIDEO_MPEG4},
#ifdef SUPPORT_CODEC_RV
    {AV_CODEC_ID_RV30, MimeType::VIDEO_RV30},
    {AV_CODEC_ID_RV40, MimeType::VIDEO_RV40},
#endif
    {AV_CODEC_ID_WMV3, MimeType::VIDEO_WMV3},
    {AV_CODEC_ID_MJPEG, MimeType::IMAGE_JPG},
    {AV_CODEC_ID_PNG, MimeType::IMAGE_PNG},
    {AV_CODEC_ID_BMP, MimeType::IMAGE_BMP},
    {AV_CODEC_ID_H263, MimeType::VIDEO_H263},
    {AV_CODEC_ID_MPEG2TS, MimeType::VIDEO_MPEG2},
    {AV_CODEC_ID_MPEG1VIDEO, MimeType::VIDEO_MPEG1},
    {AV_CODEC_ID_MPEG2VIDEO, MimeType::VIDEO_MPEG2},
    {AV_CODEC_ID_HEVC, MimeType::VIDEO_HEVC},
    {AV_CODEC_ID_VVC, MimeType::VIDEO_VVC},
    {AV_CODEC_ID_VP8, MimeType::VIDEO_VP8},
    {AV_CODEC_ID_VP9, MimeType::VIDEO_VP9},
    {AV_CODEC_ID_MSVIDEO1, MimeType::VIDEO_MSVIDEO1},
#ifdef SUPPORT_CODEC_VC1
    {AV_CODEC_ID_VC1, MimeType::VIDEO_VC1},
#endif
    {AV_CODEC_ID_AV1, MimeType::VIDEO_AV1},

    {AV_CODEC_ID_AVS3DA, MimeType::AUDIO_AVS3DA},
    {AV_CODEC_ID_APE, MimeType::AUDIO_APE},
    {AV_CODEC_ID_PCM_MULAW, MimeType::AUDIO_G711MU},
    {AV_CODEC_ID_PCM_ALAW, MimeType::AUDIO_G711A},
    {AV_CODEC_ID_GSM_MS, MimeType::AUDIO_GSM_MS},
    {AV_CODEC_ID_GSM, MimeType::AUDIO_GSM},
    {AV_CODEC_ID_WMAV1, MimeType::AUDIO_WMAV1},
    {AV_CODEC_ID_WMAV2, MimeType::AUDIO_WMAV2},
    {AV_CODEC_ID_WMAPRO, MimeType::AUDIO_WMAPRO},
    {AV_CODEC_ID_ILBC, MimeType::AUDIO_ILBC},
    {AV_CODEC_ID_TRUEHD, MimeType::AUDIO_TRUEHD},
#ifdef SUPPORT_CODEC_COOK
    {AV_CODEC_ID_COOK, MimeType::AUDIO_COOK},
#endif
    {AV_CODEC_ID_AC3, MimeType::AUDIO_AC3},
#ifdef SUPPORT_DEMUXER_EAC3
    {AV_CODEC_ID_EAC3, MimeType::AUDIO_EAC3},
#endif
    {AV_CODEC_ID_ALAC, MimeType::AUDIO_ALAC},
    {AV_CODEC_ID_SUBRIP, MimeType::TEXT_SUBRIP},
    {AV_CODEC_ID_WEBVTT, MimeType::TEXT_WEBVTT},
#ifdef SUPPORT_DEMUXER_LRC
    {AV_CODEC_ID_TEXT, MimeType::TEXT_LRC},
#endif
#ifdef SUPPORT_DEMUXER_SAMI
    {AV_CODEC_ID_SAMI, MimeType::TEXT_SAMI},
#endif
#ifdef SUPPORT_DEMUXER_ASS
    {AV_CODEC_ID_ASS, MimeType::TEXT_ASS},
#endif
    {AV_CODEC_ID_FFMETADATA, MimeType::TIMED_METADATA}
};

static std::map<std::string, FileType> g_convertFfmpegFileType = {
    {"mpegts", FileType::MPEGTS},
    {"matroska,webm", FileType::MKV},
    {"amr", FileType::AMR},
    {"amrnb", FileType::AMR},
    {"amrwb", FileType::AMR},
    {"aac", FileType::AAC},
    {"mp3", FileType::MP3},
    {"flac", FileType::FLAC},
    {"ogg", FileType::OGG},
    {"wav", FileType::WAV},
    {"flv", FileType::FLV},
    {"avi", FileType::AVI},
    {"mpeg", FileType::MPEGPS},
    {"rm", FileType::RM},
    {"ac3", FileType::AC3},
    {"wmv", FileType::WMV},
    {"asf", FileType::WMV},
    {"vob", FileType::VOB},
    {"ape", FileType::APE},
    {"srt", FileType::SRT},
    {"webvtt", FileType::VTT},
#ifdef SUPPORT_DEMUXER_LRC
    {"lrc", FileType::LRC},
#endif
#ifdef SUPPORT_DEMUXER_SAMI
    {"sami", FileType::SAMI},
#endif
#ifdef SUPPORT_DEMUXER_ASS
    {"ass", FileType::ASS},
#endif
#ifdef SUPPORT_DEMUXER_EAC3
    {"eac3", FileType::EAC3},
#endif
};

static std::map<std::string, TagType> g_formatToString = {
    {"title",         Tag::MEDIA_TITLE},
    {"artist",        Tag::MEDIA_ARTIST},
    {"album",         Tag::MEDIA_ALBUM},
    {"album_artist",  Tag::MEDIA_ALBUM_ARTIST},
    {"date",          Tag::MEDIA_DATE},
    {"comment",       Tag::MEDIA_COMMENT},
    {"genre",         Tag::MEDIA_GENRE},
    {"copyright",     Tag::MEDIA_COPYRIGHT},
    {"language",      Tag::MEDIA_LANGUAGE},
    {"description",   Tag::MEDIA_DESCRIPTION},
    {"lyrics",        Tag::MEDIA_LYRICS},
    {"author",        Tag::MEDIA_AUTHOR},
    {"composer",      Tag::MEDIA_COMPOSER},
    {"creation_time", Tag::MEDIA_CREATION_TIME},
    {"aigc",          Tag::MEDIA_AIGC}
};

static std::map<std::string, TagType> g_trackLevelBaseInfo = {
    {"title",         Tag::MEDIA_TITLE},
    {"artist",        Tag::MEDIA_ARTIST},
    {"album",         Tag::MEDIA_ALBUM},
};

static const char* g_gltfKeys[] = {
    "meta_hdlr",
    "meta_iloc",
    "meta_iinf",
    "meta_idat",
    "is_hdlr_type_gltf",
};

std::vector<TagType> g_supportSourceFormat = {
    Tag::MEDIA_TITLE,
    Tag::MEDIA_ARTIST,
    Tag::MEDIA_ALBUM,
    Tag::MEDIA_ALBUM_ARTIST,
    Tag::MEDIA_DATE,
    Tag::MEDIA_COMMENT,
    Tag::MEDIA_GENRE,
    Tag::MEDIA_COPYRIGHT,
    Tag::MEDIA_LANGUAGE,
    Tag::MEDIA_DESCRIPTION,
    Tag::MEDIA_LYRICS,
    Tag::MEDIA_AUTHOR,
    Tag::MEDIA_COMPOSER,
    Tag::MEDIA_CREATION_TIME,
    Tag::MEDIA_AIGC
};

std::vector<FileType> g_typeForTrackLevelBaseInfo = {
    FileType::OGG
};

std::vector<std::string> SplitByChar(const char* str, const char* pattern)
{
    FALSE_RETURN_V_NOLOG(str != nullptr && pattern != nullptr, {});
    std::string tempStr(str);
    std::stringstream strStream(tempStr);
    std::vector<std::string> resultVec;
    std::string item;
    while (std::getline(strStream, item, *pattern)) {
        if (!item.empty()) {
            resultVec.push_back(item);
        }
    }
    MEDIA_LOG_D("Split by [" PUBLIC_LOG_S "], get " PUBLIC_LOG_ZU " string", pattern, resultVec.size());
    return resultVec;
}

void DumpFileInfo(const AVFormatContext& avFormatContext)
{
    FALSE_RETURN_MSG(avFormatContext.iformat != nullptr, "Iformat is nullptr");
    MEDIA_LOG_D("File name [" PUBLIC_LOG_S "]", avFormatContext.iformat->name);
    if (StartWith(avFormatContext.iformat->name, "mov,mp4,m4a")) {
        const AVDictionaryEntry *type = av_dict_get(avFormatContext.metadata, "major_brand", NULL, 0);
        if (type == nullptr) {
            MEDIA_LOG_D("Not found ftyp");
        } else {
            MEDIA_LOG_D("Major brand: " PUBLIC_LOG_S, type->value);
        }
    }
}

std::string RemoveDuplication(const std::string origin)
{
    FALSE_RETURN_V_NOLOG(origin.find(";") != std::string::npos, origin);
    std::vector<std::string> subStrings = SplitByChar(origin.c_str(), ";");
    FALSE_RETURN_V_NOLOG(subStrings.size() > 1, origin);

    std::string outString;
    std::vector<std::string> uniqueSubStrings;
    for (auto str : subStrings) {
        if (std::count(uniqueSubStrings.begin(), uniqueSubStrings.end(), str) == 0) {
            uniqueSubStrings.push_back(str);
        }
    }
    for (size_t idx = 0; idx < uniqueSubStrings.size(); idx++) {
        outString += uniqueSubStrings[idx];
        if (idx < uniqueSubStrings.size() - 1) {
            outString += ";";
        }
    }
    MEDIA_LOG_D("[%{public}s]->[%{public}s]", origin.c_str(), outString.c_str());
    return outString;
}

std::string ToLower(const std::string& str)
{
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return (c == '_') ? c : std::tolower(c);
    });
    MEDIA_LOG_D("[" PUBLIC_LOG_S "] -> [" PUBLIC_LOG_S "]", str.c_str(), res.c_str());
    return res;
}

bool IsUTF8Char(unsigned char chr, int32_t &nBytes)
{
    if (nBytes == 0) {
        if ((chr & MASK_80) == 0) {
            return true;
        }
        while ((chr & MASK_80) == MASK_80) {
            chr <<= 1;
            nBytes++;
        }

        if (nBytes < MIN_BYTES || nBytes > MAX_BYTES) {
            return false;
        }
        nBytes--;
    } else {
        if ((chr & MASK_C0) != MASK_80) {
            return false;
        }
        nBytes--;
    }
    return true;
}

bool IsUTF8(const std::string &data)
{
    int32_t nBytes = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        if (!IsUTF8Char(data[i], nBytes)) {
            MEDIA_LOG_D("Detect char not in uft8");
            return false;
        }
    }
    return true;
}

std::string ConvertGBKToUTF8(const std::string &strGbk)
{
    if (strGbk.length() == 0) {
        return "";
    }

    const std::string fromCharset = "gb18030";
    const std::string toCharset = "utf-8";
    iconv_t cd = iconv_open(toCharset.c_str(), fromCharset.c_str());
    if (cd == reinterpret_cast<iconv_t>(-1)) { // iconv_t is a void* type
        MEDIA_LOG_D("Call iconv_open failed");
        return "";
    }
    size_t inLen = strGbk.length();
    size_t outLen = inLen * 4; // max for chinese character
    char* inBuf = const_cast<char*>(strGbk.c_str());
    if (inBuf == nullptr) {
        MEDIA_LOG_D("Get in buffer failed");
        iconv_close(cd);
        return "";
    }
    char* outBuf = new char[outLen];
    if (outBuf == nullptr) {
        MEDIA_LOG_D("Get out buffer failed");
        iconv_close(cd);
        return "";
    }
    char* outBufBack = outBuf;
    if (iconv(cd, &inBuf, &inLen, &outBuf, &outLen) == static_cast<size_t>(-1)) { // iconv return SIZE_MAX when failed
        MEDIA_LOG_D("Call iconv failed");
        delete[] outBufBack;
        outBufBack = nullptr;
        iconv_close(cd);
        return "";
    }
    std::string strOut(outBufBack, outBuf - outBufBack);
    delete[] outBufBack;
    outBufBack = nullptr;
    iconv_close(cd);
    return strOut;
}

bool IsGBK(const char* data)
{
    int len = static_cast<int>(strlen(data));
    int i = 0;
    while (i < len) {
        if (static_cast<unsigned char>(data[i]) <= 0x7f) { // one byte encoding or ASCII
            i++;
            continue;
        } else { // double bytes encoding
            if (i + 1  < len &&
                static_cast<unsigned char>(data[i]) >= 0x81 && static_cast<unsigned char>(data[i]) <= 0xfe &&
                static_cast<unsigned char>(data[i + 1]) >= 0x40 && static_cast<unsigned char>(data[i + 1]) <= 0xfe) {
                i += DOUBLE_BYTES; // double bytes
                continue;
            } else {
                return false;
            }
        }
    }
    return true;
}

static std::vector<AVCodecID> g_imageCodecID = {
    AV_CODEC_ID_PNG,
    AV_CODEC_ID_PAM,
    AV_CODEC_ID_BMP,
    AV_CODEC_ID_JPEG2000,
    AV_CODEC_ID_TARGA,
    AV_CODEC_ID_TIFF,
    AV_CODEC_ID_GIF,
    AV_CODEC_ID_PCX,
    AV_CODEC_ID_XWD,
    AV_CODEC_ID_XBM,
    AV_CODEC_ID_WEBP,
    AV_CODEC_ID_APNG,
    AV_CODEC_ID_XPM,
    AV_CODEC_ID_SVG,
};

static std::map<std::string, VideoRotation> g_pFfRotationMap = {
    {"0", VIDEO_ROTATION_0},
    {"90", VIDEO_ROTATION_90},
    {"180", VIDEO_ROTATION_180},
    {"270", VIDEO_ROTATION_270},
};

static const std::map<std::string, VideoOrientationType> matrixTypes = {
    /**
     * display matrix
     *                                  | a b u |
     *   (a, b, u, c, d, v, x, y, w) -> | c d v |
     *                                  | x y w |
     * [a b c d] can confirm the orientation type
     */
    {"0 -1 1 0", VideoOrientationType::ROTATE_90},
    {"-1 0 0 -1", VideoOrientationType::ROTATE_180},
    {"0 1 -1 0", VideoOrientationType::ROTATE_270},
    {"-1 0 0 1", VideoOrientationType::FLIP_H},
    {"1 0 0 -1", VideoOrientationType::FLIP_V},
    {"0 1 1 0", VideoOrientationType::FLIP_H_ROT90},
    {"0 -1 -1 0", VideoOrientationType::FLIP_V_ROT90},
};

VideoOrientationType GetMatrixType(const std::string& value)
{
    auto it = matrixTypes.find(value);
    if (it!= matrixTypes.end()) {
        return it->second;
    } else {
        return VideoOrientationType::ROTATE_NONE;
    }
}

inline int ConvFp(int32_t x)
{
    return static_cast<int32_t>(x / (1 << 16)); // 16 is used for digital conversion
}

std::string ConvertArrayToString(const int* array, size_t size)
{
    std::string result;
    for (size_t i = 0; i < size; ++i) {
        if (i > 0) {
            result += ' ';
        }
        result += std::to_string(array[i]);
    }
    return result;
}

enum ValueType {
    INT32 = 0,
    FLOAT = 1,
    BUFFER = 2,
    INT64 = 3,
};

template <typename T>
bool ConvertString(const std::string &str, T &result)
{
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    return ec == std::errc{} && ptr == str.data() + str.size();
}

bool StringConverterFloat(const std::string &str, float &result)
{
    char *end = nullptr;
    errno = 0;
    result = std::strtof(str.c_str(), &end);
    return end != str.c_str() && *end == '\0' && errno == 0;
}

static int8_t HexCharToValue(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + VALUE_10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + VALUE_10;
    }
    return INVALID_VAL;
}

static bool ConvertHexStrToBuffer(const std::string hexStr, std::vector<uint8_t> &buffer)
{
    if (hexStr.empty() || hexStr.size() % VALUE_2 != 0) {
        return false;
    }
    int len = static_cast<int>(hexStr.size()) / VALUE_2;
    buffer.resize(len);
    for (int i = 0; i < len; i++) {
        int8_t high = HexCharToValue(hexStr[i * VALUE_2]);
        int8_t low = HexCharToValue(hexStr[i * VALUE_2 + POS_1]);
        FALSE_RETURN_V_MSG_E(high >= 0 && low >= 0, false,
            "invalid hexChar" PUBLIC_LOG_U8 " " PUBLIC_LOG_U8, hexStr[i * VALUE_2], hexStr[i * VALUE_2 + POS_1]);
        buffer[i] = (static_cast<uint8_t>(high) << VALUE_4) | static_cast<uint8_t>(low);
    }
    return true;
}

static void SetToFormatIfConvertSuccess(Meta& format, const TagType& tag, std::string valueStr, ValueType valueType)
{
    bool convertSuccess = false;
    switch (valueType) {
        case ValueType::INT32: {
            int32_t int32Value = -1;
            if (ConvertString(valueStr, int32Value)) {
                format.SetData<int32_t>(tag, int32Value);
                convertSuccess = true;
            }
            break;
        }
        case ValueType::FLOAT: {
            float floatValue = -1;
            if (StringConverterFloat(valueStr, floatValue)) {
                format.SetData<float>(tag, floatValue);
                convertSuccess = true;
            }
            break;
        }
        case ValueType::BUFFER: {
            std::vector<uint8_t> buffer;
            if (ConvertHexStrToBuffer(valueStr, buffer)) {
                format.SetData(tag, buffer);
                convertSuccess = true;
            }
            break;
        }
        case ValueType::INT64: {
            int64_t int64Value = -1;
            if (ConvertString(valueStr, int64Value)) {
                format.SetData<int64_t>(tag, int64Value);
                convertSuccess = true;
            }
            break;
        }
        default:
            MEDIA_LOG_W("Unsupport value type " PUBLIC_LOG_D32, static_cast<int32_t>(valueType));
            break;
    }
    if (!convertSuccess) {
        MEDIA_LOG_W("Parse failed key[" PUBLIC_LOG_S "] value[" PUBLIC_LOG_S "] type[" PUBLIC_LOG_D32 "]",
            std::string(tag).c_str(), valueStr.c_str(), static_cast<int32_t>(valueType));
    }
}

bool IsPCMStream(AVCodecID codecID)
{
    MEDIA_LOG_D("CodecID " PUBLIC_LOG_D32 "[" PUBLIC_LOG_S "]",
        static_cast<int32_t>(codecID), avcodec_get_name(codecID));
    return StartWith(avcodec_get_name(codecID), "pcm_");
}

int64_t GetDefaultTrackStartTime(const AVFormatContext& avFormatContext)
{
    int64_t defaultTime = 0;
    for (uint32_t trackIndex = 0; trackIndex < avFormatContext.nb_streams; ++trackIndex) {
        auto avStream = avFormatContext.streams[trackIndex];
        if (avStream != nullptr && avStream->codecpar != nullptr &&
            avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && avStream->start_time != AV_NOPTS_VALUE) {
            defaultTime = AvTime2Us(ConvertTimeFromFFmpeg(avStream->start_time, avStream->time_base));
        }
    }
    return defaultTime;
}

static int FfAv3aGetNbObjects(AVChannelLayout *channelLayout)
{
    int nbObjects = 0;
    if (channelLayout->order != AV_CHANNEL_ORDER_CUSTOM) {
        return 0;
    }
    for (int i = 0; i < channelLayout->nb_channels; i++) {
        if (channelLayout->u.map[i].id == AV3A_CH_AUDIO_OBJECT) {
            nbObjects++;
        }
    }
    return nbObjects;
}

static uint64_t FfAv3aGetChannelLayoutMask(AVChannelLayout *channelLayout)
{
    uint64_t mask = 0L;
    if (channelLayout->order != AV_CHANNEL_ORDER_CUSTOM) {
        return 0;
    }
    for (int i = 0; i < channelLayout->nb_channels; i++) {
        if (channelLayout->u.map[i].id == AV3A_CH_AUDIO_OBJECT) {
            return mask;
        }
        mask |= (1ULL << channelLayout->u.map[i].id);
    }
    return mask;
}

void FFmpegFormatHelper::ParseTrackType(const AVFormatContext& avFormatContext, Meta& format)
{
    format.Set<Tag::MEDIA_TRACK_COUNT>(static_cast<int32_t>(avFormatContext.nb_streams));
    format.Set<Tag::MEDIA_HAS_VIDEO>(false);
    format.Set<Tag::MEDIA_HAS_AUDIO>(false);
    format.Set<Tag::MEDIA_HAS_SUBTITLE>(false);
    format.Set<Tag::MEDIA_HAS_TIMEDMETA>(false);
    format.Set<Tag::MEDIA_HAS_AUXILIARY>(false);
    for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
        if (avFormatContext.streams[i] == nullptr || avFormatContext.streams[i]->codecpar == nullptr) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is invalid", i);
            continue;
        }
        if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!IsImageTrack(*(avFormatContext.streams[i]))) {
                format.Set<Tag::MEDIA_HAS_VIDEO>(true);
            }
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            format.Set<Tag::MEDIA_HAS_AUDIO>(true);
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            format.Set<Tag::MEDIA_HAS_SUBTITLE>(true);
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_TIMEDMETA) {
            format.Set<Tag::MEDIA_HAS_TIMEDMETA>(true);
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUXILIARY) {
            format.Set<Tag::MEDIA_HAS_AUXILIARY>(true);
        }
    }
}

void FFmpegFormatHelper::ParseMediaInfo(const AVFormatContext& avFormatContext, Meta& format)
{
    ParseTrackType(avFormatContext, format);
    DumpFileInfo(avFormatContext);
    format.Set<Tag::MEDIA_FILE_TYPE>(GetFileTypeByName(avFormatContext));
    int64_t duration = avFormatContext.duration;
    if (duration == AV_NOPTS_VALUE) {
        duration = 0;
        const AVDictionaryEntry *metaDuration = av_dict_get(avFormatContext.metadata, "DURATION", NULL, 0);
        int64_t us;
        if (metaDuration != nullptr && (av_parse_time(&us, metaDuration->value, 1) == 0)) {
            if (us > duration) {
                duration = us;
            }
        }
    }
    if (duration > 0 && duration != AV_NOPTS_VALUE) {
        format.Set<Tag::MEDIA_DURATION>(static_cast<int64_t>(duration));
    }
    if (avFormatContext.start_time != AV_NOPTS_VALUE) {
        format.Set<Tag::MEDIA_CONTAINER_START_TIME>(static_cast<int64_t>(avFormatContext.start_time));
    } else {
        format.Set<Tag::MEDIA_CONTAINER_START_TIME>(static_cast<int64_t>(0));
        MEDIA_LOG_W("Parse container start time failed");
    }
    ParseLocationInfo(avFormatContext, format);
    ParseInfoFromMetadata(avFormatContext.metadata, format, g_formatToString);
    ParseGltfInfo(avFormatContext, format);
}

void FFmpegFormatHelper::ParseLocationInfo(const AVFormatContext& avFormatContext, Meta &format)
{
    MEDIA_LOG_D("Parse location info");
    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avFormatContext.metadata, "location", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        valPtr = av_dict_get(avFormatContext.metadata, "LOCATION", nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        MEDIA_LOG_D("Parse failed");
        return;
    }
    MEDIA_LOG_D("Get location string successfully: %{private}s", valPtr->value);
    std::string locationStr = std::string(valPtr->value);
    std::regex pattern(R"([\+\-]\d+\.\d+)");
    std::sregex_iterator numbers(locationStr.cbegin(), locationStr.cend(), pattern);
    std::sregex_iterator end;
    // at least contain latitude and longitude
    if (static_cast<uint32_t>(std::distance(numbers, end)) < VALID_LOCATION_LEN) {
        MEDIA_LOG_D("Info format error");
        return;
    }
    SetToFormatIfConvertSuccess(format, Tag::MEDIA_LATITUDE, numbers->str(), ValueType::FLOAT);
    SetToFormatIfConvertSuccess(format, Tag::MEDIA_LONGITUDE, (++numbers)->str(), ValueType::FLOAT);
}

void FFmpegFormatHelper::ParseUserMeta(const AVFormatContext& avFormatContext, std::shared_ptr<Meta> format)
{
    MEDIA_LOG_D("Parse user data info");
    AVDictionaryEntry *valPtr = nullptr;
    while ((valPtr = av_dict_get(avFormatContext.metadata, "", valPtr, AV_DICT_IGNORE_SUFFIX)))  {
        if (StartWith(valPtr->key, "moov_level_meta_key_")) {
            MEDIA_LOG_D("Ffmpeg key: " PUBLIC_LOG_S, (valPtr->key));
            if (strlen(valPtr->value) <= VALUE_PREFIX_LEN) {
                MEDIA_LOG_D("Parse user data info " PUBLIC_LOG_S " failed, value too short", valPtr->key);
                continue;
            }
            if (StartWith(valPtr->value, "00000000")) { // reserved
                MEDIA_LOG_D("Key: " PUBLIC_LOG_S " | type: reserved", (valPtr->key + KEY_PREFIX_LEN));
                SetToFormatIfConvertSuccess(
                    *format, valPtr->key + KEY_PREFIX_LEN, valPtr->value + VALUE_PREFIX_LEN, ValueType::BUFFER);
            } else if (StartWith(valPtr->value, "00000001")) { // string
                MEDIA_LOG_D("Key: " PUBLIC_LOG_S " | type: string", (valPtr->key + KEY_PREFIX_LEN));
                format->SetData(valPtr->key + KEY_PREFIX_LEN, std::string(valPtr->value + VALUE_PREFIX_LEN));
            } else if (StartWith(valPtr->value, "00000017")) { // float
                MEDIA_LOG_D("Key: " PUBLIC_LOG_S " | type: float", (valPtr->key + KEY_PREFIX_LEN));
                SetToFormatIfConvertSuccess(
                    *format, valPtr->key + KEY_PREFIX_LEN, valPtr->value + VALUE_PREFIX_LEN, ValueType::FLOAT);
            } else if (StartWith(valPtr->value, "00000043") || StartWith(valPtr->value, "00000015")) { // int
                MEDIA_LOG_D("Key: " PUBLIC_LOG_S " | type: int", (valPtr->key + KEY_PREFIX_LEN));
                SetToFormatIfConvertSuccess(
                    *format, valPtr->key + KEY_PREFIX_LEN, valPtr->value + VALUE_PREFIX_LEN, ValueType::INT32);
            } else { // unknow
                MEDIA_LOG_D("Key: " PUBLIC_LOG_S " | type: unknow", (valPtr->key + KEY_PREFIX_LEN));
                format->SetData(valPtr->key + KEY_PREFIX_LEN, std::string(valPtr->value + VALUE_PREFIX_LEN));
            }
        }
    }
}

void FFmpegFormatHelper::ParseTrackInfo(const AVStream& avStream, Meta& format, const AVFormatContext& avFormatContext)
{
    FALSE_RETURN_MSG(avStream.codecpar != nullptr, "Codecpar is nullptr");
    ParseBaseTrackInfo(avStream, format, avFormatContext);
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (IsImageTrack(avStream)) {
            ParseImageTrackInfo(avStream, format);
        } else {
            ParseAVTrackInfo(avStream, format);
            ParseVideoTrackInfo(avStream, format, avFormatContext);
        }
    } else if (avStream.codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        ParseAVTrackInfo(avStream, format);
        ParseAudioTrackInfo(avStream, format, avFormatContext);
    } else if (avStream.codecpar->codec_type == AVMEDIA_TYPE_TIMEDMETA) {
        ParseAVTrackInfo(avStream, format);
        ParseTimedMetaTrackInfo(avStream, format);
        ParseAuxiliaryTrackInfo(avStream, format);
    } else if (avStream.codecpar->codec_type == AVMEDIA_TYPE_AUXILIARY) {
        ParseAVTrackInfo(avStream, format);
        ParseAuxiliaryTrackInfo(avStream, format);
        if (IsVideoType(avStream)) {
            ParseVideoTrackInfo(avStream, format, avFormatContext);
        } else if (IsAudioType(avStream)) {
            ParseAudioTrackInfo(avStream, format, avFormatContext);
        }
    }
    FileType fileType = GetFileTypeByName(avFormatContext);
    if (std::count(g_typeForTrackLevelBaseInfo.begin(), g_typeForTrackLevelBaseInfo.end(), fileType) > 0) {
        ParseInfoFromMetadata(avStream.metadata, format, g_trackLevelBaseInfo);
    }
}

void FFmpegFormatHelper::ParseBaseTrackInfo(const AVStream& avStream, Meta &format,
                                            const AVFormatContext& avFormatContext)
{
    if (g_codecIdToMime.count(avStream.codecpar->codec_id) != 0) {
        format.Set<Tag::MIME_TYPE>(std::string(g_codecIdToMime[avStream.codecpar->codec_id]));
    } else if (IsPCMStream(avStream.codecpar->codec_id)) {
        format.Set<Tag::MIME_TYPE>(std::string(MimeType::AUDIO_RAW));
    } else {
        format.Set<Tag::MIME_TYPE>(std::string(MimeType::INVALID_TYPE));
        MEDIA_LOG_W("Parse mime type failed: " PUBLIC_LOG_S, avcodec_get_name(avStream.codecpar->codec_id));
    }
    format.Set<Tag::ORIGINAL_CODEC_NAME>(std::string(avcodec_get_name(avStream.codecpar->codec_id)));

    AVMediaType mediaType = avStream.codecpar->codec_type;
    if (g_convertFfmpegTrackType.count(mediaType) > 0) {
        format.Set<Tag::MEDIA_TYPE>(g_convertFfmpegTrackType[mediaType]);
    } else {
        MEDIA_LOG_W("Parse track type failed: " PUBLIC_LOG_D32, static_cast<int32_t>(mediaType));
    }

    if (avStream.start_time != AV_NOPTS_VALUE) {
        format.SetData(Tag::MEDIA_START_TIME,
            AvTime2Us(ConvertTimeFromFFmpeg(avStream.start_time, avStream.time_base)));
    } else {
        if (mediaType == AVMEDIA_TYPE_AUDIO || mediaType == AVMEDIA_TYPE_AUXILIARY) {
            format.SetData(Tag::MEDIA_START_TIME, GetDefaultTrackStartTime(avFormatContext));
        }
        MEDIA_LOG_W("Parse track start time failed");
    }
}

static bool IsWmaFormat(const AVFormatContext& avFormatContext)
{
    for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
        if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return false;
        }
    }
    return true;
}

FileType FFmpegFormatHelper::GetFileTypeByName(const AVFormatContext& avFormatContext)
{
    FALSE_RETURN_V_MSG_E(avFormatContext.iformat != nullptr, FileType::UNKNOW, "Iformat is nullptr");
    const char *fileName = avFormatContext.iformat->name;
    FileType fileType = FileType::UNKNOW;
    if (StartWith(fileName, "mov,mp4,m4a")) {
        const AVDictionaryEntry *type = av_dict_get(avFormatContext.metadata, "major_brand", NULL, 0);
        if (type == nullptr) {
            return FileType::MP4;
        }
        if (StartWith(type->value, "m4a") || StartWith(type->value, "M4A")) {
            fileType = FileType::M4A;
        } else if (StartWith(type->value, "m4v") || StartWith(type->value, "M4V")) {
            fileType = FileType::M4V;
        } else if (StartWith(type->value, "3g2")) {
            fileType = FileType::FT_3G2;
        } else if (StartWith(type->value, "3gp")) {
            fileType = FileType::FT_3GP;
        } else if (StartWith(type->value, "qt") || StartWith(type->value, "QT")) {
            fileType = FileType::MOV;
        } else {
            fileType = FileType::MP4;
        }
    } else if (StartWith(fileName, "asf") && IsWmaFormat(avFormatContext)) {
        fileType = FileType::WMA;
    } else {
        if (g_convertFfmpegFileType.count(fileName) != 0) {
            fileType = g_convertFfmpegFileType[fileName];
        }
    }
    return fileType;
}

void FFmpegFormatHelper::ParseAVTrackInfo(const AVStream& avStream, Meta &format)
{
    int64_t bitRate = static_cast<int64_t>(avStream.codecpar->bit_rate);
    if (bitRate > 0) {
        format.Set<Tag::MEDIA_BITRATE>(bitRate);
    } else {
        MEDIA_LOG_D("Parse bitrate failed: " PUBLIC_LOG_D64, bitRate);
    }

    if (avStream.codecpar->extradata_size > 0 && avStream.codecpar->extradata != nullptr) {
        std::vector<uint8_t> extra(avStream.codecpar->extradata_size);
        extra.assign(avStream.codecpar->extradata, avStream.codecpar->extradata + avStream.codecpar->extradata_size);
        format.Set<Tag::MEDIA_CODEC_CONFIG>(extra);
    } else {
        MEDIA_LOG_D("Parse codec config failed");
    }
    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avStream.metadata, "language", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr != nullptr) {
        format.SetData(Tag::MEDIA_LANGUAGE, std::string(valPtr->value));
    } else {
        MEDIA_LOG_D("Parse track language failed");
    }
}

void FFmpegFormatHelper::ParseVideoTrackInfo(const AVStream& avStream, Meta &format,
                                             const AVFormatContext& avFormatContext)
{
    format.Set<Tag::VIDEO_WIDTH>(static_cast<uint32_t>(avStream.codecpar->width));
    format.Set<Tag::VIDEO_HEIGHT>(static_cast<uint32_t>(avStream.codecpar->height));
    format.Set<Tag::VIDEO_DELAY>(static_cast<uint32_t>(avStream.codecpar->video_delay));

    double frameRate = 0;
    if (avStream.avg_frame_rate.den == 0 || avStream.avg_frame_rate.num == 0) {
        frameRate = static_cast<double>(av_q2d(avStream.r_frame_rate));
    } else {
        frameRate = static_cast<double>(av_q2d(avStream.avg_frame_rate));
    }
    if (frameRate > 0) {
        format.Set<Tag::VIDEO_FRAME_RATE>(frameRate);
    } else {
        MEDIA_LOG_D("Parse frame rate failed: " PUBLIC_LOG_F, frameRate);
    }

    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avStream.metadata, "rotate", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        valPtr = av_dict_get(avStream.metadata, "ROTATE", nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        MEDIA_LOG_D("Parse rotate info from meta failed");
        ParseRotationFromMatrix(avStream, format);
    } else {
        if (g_pFfRotationMap.count(std::string(valPtr->value)) > 0) {
            format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap[std::string(valPtr->value)]);
        } else {
            MEDIA_LOG_D("Unsupport rotate " PUBLIC_LOG_S, valPtr->value);
        }
    }
    FileType fileType = GetFileTypeByName(avFormatContext);
    if (IsMpeg4File(fileType)) {
        ParseOrientationFromMatrix(avStream, format);
    }

    AVRational sar = av_guess_sample_aspect_ratio(&const_cast<AVFormatContext&>(avFormatContext),
        &const_cast<AVStream&>(avStream), nullptr);
    if (sar.num && sar.den) {
        format.Set<Tag::VIDEO_SAR>(static_cast<double>(av_q2d(sar)));
    }

    if (avStream.codecpar->codec_id == AV_CODEC_ID_HEVC) {
        ParseHvccBoxInfo(avStream, format);
        ParseColorBoxInfo(avStream, format);
    }
}

void FFmpegFormatHelper::ParseRotationFromMatrix(const AVStream& avStream, Meta &format)
{
    int32_t *displayMatrix = (int32_t *)av_stream_get_side_data(&avStream, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    if (displayMatrix) {
        float rotation = -round(av_display_rotation_get(displayMatrix));
        MEDIA_LOG_D("Parse rotate info from display matrix: " PUBLIC_LOG_F, rotation);
        if (isnan(rotation)) {
            format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["0"]);
            return;
        } else if (rotation < 0) {
            rotation += VIDEO_ROTATION_360;
        }
        switch (int(rotation)) {
            case VIDEO_ROTATION_90:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["90"]);
                break;
            case VIDEO_ROTATION_180:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["180"]);
                break;
            case VIDEO_ROTATION_270:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["270"]);
                break;
            default:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["0"]);
                break;
        }
    } else {
        MEDIA_LOG_D("Parse rotate info from display matrix failed, set rotation as default 0");
        format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["0"]);
    }
}

void PrintMatrixToLog(int32_t * matrix, const std::string& matrixName)
{
    MEDIA_LOG_D(PUBLIC_LOG_S ": [" PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " "
            PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 "]",
            matrixName.c_str(), matrix[0], matrix[1], matrix[2], matrix[3], matrix[4],
            matrix[5], matrix[6], matrix[7], matrix[8]);
}

void FFmpegFormatHelper::ParseOrientationFromMatrix(const AVStream& avStream, Meta &format)
{
    VideoOrientationType orientationType = VideoOrientationType::ROTATE_NONE;
    int32_t *displayMatrix = (int32_t *)av_stream_get_side_data(&avStream, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    if (displayMatrix) {
        PrintMatrixToLog(displayMatrix, "displayMatrix");
        int convertedMatrix[CONVERT_MATRIX_SIZE] = {0, 0, 0, 0};
        std::transform(&displayMatrix[0], &displayMatrix[0] + 1, // 0 is displayMatrix index, 1 is copy lenth
                       &convertedMatrix[0], ConvFp); // 0 is convertedMatrix index
        std::transform(&displayMatrix[1], &displayMatrix[1] + 1, // 1 is displayMatrix index, 1 is copy lenth
                       &convertedMatrix[1], ConvFp); // 1 is convertedMatrix index
        std::transform(&displayMatrix[3], &displayMatrix[3] + 1, // 3 is displayMatrix index, 1 is copy lenth
                       &convertedMatrix[2], ConvFp); // 2 is convertedMatrix index
        std::transform(&displayMatrix[4], &displayMatrix[4] + 1, // 4 is displayMatrix index, 1 is copy lenth
                       &convertedMatrix[3], ConvFp); // 3 is convertedMatrix index
        orientationType = GetMatrixType(ConvertArrayToString(convertedMatrix, CONVERT_MATRIX_SIZE));
    } else {
        MEDIA_LOG_D("Parse orientation info from display matrix failed, set orientation as dafault 0");
    }
    format.Set<Tag::VIDEO_ORIENTATION_TYPE>(orientationType);
    MEDIA_LOG_D("Type of matrix is: " PUBLIC_LOG_D32, static_cast<int>(orientationType));
}

void FFmpegFormatHelper::ParseImageTrackInfo(const AVStream& avStream, Meta &format)
{
    format.Set<Tag::VIDEO_WIDTH>(static_cast<uint32_t>(avStream.codecpar->width));
    format.Set<Tag::VIDEO_HEIGHT>(static_cast<uint32_t>(avStream.codecpar->height));
    AVPacket pkt = avStream.attached_pic;
    if (pkt.size > 0 && pkt.data != nullptr) {
        std::vector<uint8_t> cover(pkt.size);
        cover.assign(pkt.data, pkt.data + pkt.size);
        format.Set<Tag::MEDIA_COVER>(cover);
    } else {
        MEDIA_LOG_D("Parse cover failed: " PUBLIC_LOG_D32, pkt.size);
    }
}

void FFmpegFormatHelper::ParseAudioTrackInfo(const AVStream& avStream, Meta &format,
                                             const AVFormatContext& avFormatContext)
{
    int sampleRate = avStream.codecpar->sample_rate;
    int channels = avStream.codecpar->channels;
    if (channels <= 0) {
        channels = avStream.codecpar->ch_layout.nb_channels;
    }
    int frameSize = avStream.codecpar->frame_size;
    if (sampleRate > 0) {
        format.Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<uint32_t>(sampleRate));
    } else {
        MEDIA_LOG_D("Parse sample rate failed: " PUBLIC_LOG_D32, sampleRate);
    }
    if (channels > 0) {
        format.Set<Tag::AUDIO_OUTPUT_CHANNELS>(static_cast<uint32_t>(channels));
        format.Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<uint32_t>(channels));
    } else {
        MEDIA_LOG_D("Parse channel count failed: " PUBLIC_LOG_D32, channels);
    }
    if (frameSize > 0) {
        format.Set<Tag::AUDIO_SAMPLE_PER_FRAME>(static_cast<uint32_t>(frameSize));
    } else {
        MEDIA_LOG_D("Parse frame rate failed: " PUBLIC_LOG_D32, frameSize);
    }
    AudioChannelLayout channelLayout = FFMpegConverter::ConvertFFToOHAudioChannelLayoutV2(
        avStream.codecpar->channel_layout, channels);
    format.Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(channelLayout);
    format.Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);

    AudioSampleFormat fmt = (IsPCMStream(avStream.codecpar->codec_id)) ?
        FFMpegConverter::ConvertFFMpegAVCodecIdToOHAudioFormat(avStream.codecpar->codec_id) :
        FFMpegConverter::ConvertFFMpegToOHAudioFormat(static_cast<AVSampleFormat>(avStream.codecpar->format));
    format.Set<Tag::AUDIO_SAMPLE_FORMAT>(fmt);

    if (avStream.codecpar->codec_id == AV_CODEC_ID_AAC) {
        format.Set<Tag::AUDIO_AAC_IS_ADTS>(1);
    } else if (avStream.codecpar->codec_id == AV_CODEC_ID_AAC_LATM) {
        format.Set<Tag::AUDIO_AAC_IS_ADTS>(0);
    }
    format.Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(avStream.codecpar->bits_per_coded_sample);
    format.Set<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(avStream.codecpar->bits_per_raw_sample);
    format.Set<Tag::AUDIO_BLOCK_ALIGN>(avStream.codecpar->block_align);

    if (avStream.codecpar->codec_id == AV_CODEC_ID_AVS3DA) {
        format.Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::UNKNOWN);
        format.Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::UNKNOWN);
        if (avStream.codecpar->ch_layout.order == AV_CHANNEL_ORDER_CUSTOM ||
            avStream.codecpar->ch_layout.order == AV_CHANNEL_ORDER_AMBISONIC) {
            ParseAv3aInfo(avStream, format);
        }
        ConvertAv3aSampleFormat(avStream, format);
    }

    ParseAudioApeTrackInfo(avStream, format, avFormatContext);
}

void FFmpegFormatHelper::ParseAudioApeTrackInfo(const AVStream& avStream, Meta &format,
                                                const AVFormatContext& avFormatContext)
{
    if (GetFileTypeByName(avFormatContext) == FileType::APE) {
        // Ffmpeg ensures that the value is definitely Int type
        const AVDictionaryEntry *meta = av_dict_get(avFormatContext.metadata, "max_frame_size", NULL, 0);
        if (meta != nullptr) {
            int64_t maxFrameSize = -1;
            if (ConvertString<int64_t>(meta->value, maxFrameSize)) {
                FALSE_RETURN_MSG(maxFrameSize >= INT32_MIN && maxFrameSize <= INT32_MAX, "Parse max frame size failed");
                format.Set<Tag::AUDIO_MAX_INPUT_SIZE>(static_cast<int32_t>(maxFrameSize));
            } else {
                MEDIA_LOG_W("error frameSize " PUBLIC_LOG_S, meta->value);
            }
        }
        meta = av_dict_get(avFormatContext.metadata, "sample_per_frame", NULL, 0);
        if (meta != nullptr) {
            int64_t samplePerFrame = -1;
            if (ConvertString<int64_t>(meta->value, samplePerFrame)) {
                FALSE_RETURN_MSG(samplePerFrame >= INT32_MIN && samplePerFrame <= INT32_MAX,
                    "Parse sample per frame failed");
                format.Set<Tag::AUDIO_SAMPLE_PER_FRAME>(static_cast<int32_t>(samplePerFrame));
            } else {
                MEDIA_LOG_W("error sample per frame " PUBLIC_LOG_S, meta->value);
            }
        }
    }
}

void FFmpegFormatHelper::ConvertAv3aSampleFormat(const AVStream& avStream, Meta &format)
{
    AudioSampleFormat fmt;
    switch (avStream.codecpar->bits_per_raw_sample) {
        case AV3A_SAMPLE_8BITS: // 8 bits
            fmt = AudioSampleFormat::SAMPLE_U8;
            break;
        case AV3A_SAMPLE_16BITS: // 16 bits
            fmt = AudioSampleFormat::SAMPLE_S16LE;
            break;
        case AV3A_SAMPLE_24BITS: // 24 bits
            fmt = AudioSampleFormat::SAMPLE_S24LE;
            break;
        default:
            fmt = AudioSampleFormat::INVALID_WIDTH;
            break;
    }
    format.Set<Tag::AUDIO_SAMPLE_FORMAT>(fmt);
}

void FFmpegFormatHelper::ParseAv3aInfo(const AVStream& avStream, Meta &format)
{
    int channels = avStream.codecpar->channels; // 总通道数
    AudioChannelLayout channelLayout = AudioChannelLayout::UNKNOWN;
    int objectNumber = 0; // 对象数量
    uint64_t channelLayoutMask = 0L;
    if (avStream.codecpar->ch_layout.order == AV_CHANNEL_ORDER_CUSTOM) {
        // 获取mask（如果是纯对象模式则不包含声场，返回0L）
        channelLayoutMask = FfAv3aGetChannelLayoutMask(&avStream.codecpar->ch_layout);
        objectNumber = FfAv3aGetNbObjects(&avStream.codecpar->ch_layout); // 获取对象数量，如果不包含对象返回0
        if (channelLayoutMask > 0L) {
            channelLayout = FFMpegConverter::ConvertAudioVividToOHAudioChannelLayout(
                channelLayoutMask, channels - objectNumber);
            if (channelLayoutMask != static_cast<uint64_t>(channelLayout)) {
                MEDIA_LOG_W("Get channel layout failed, use default channel layout");
            }
        } else {
            channelLayout = AudioChannelLayout::UNKNOWN;
        }
    } else if (avStream.codecpar->ch_layout.order == AV_CHANNEL_ORDER_AMBISONIC) {
        int hoaOrder = static_cast<int>(sqrt(channels)) - 1;
        if (hoaOrder == 1) {
            channelLayout = AudioChannelLayout::HOA_ORDER1_ACN_SN3D;
        } else if (hoaOrder == 2) { // hoaOrder is 2
            channelLayout = AudioChannelLayout::HOA_ORDER2_ACN_SN3D;
        } else if (hoaOrder == 3) { // hoaOrder is 3
            channelLayout = AudioChannelLayout::HOA_ORDER3_ACN_SN3D;
        } else {
            MEDIA_LOG_W("Get hoa order failed");
        }
        format.Set<Tag::AUDIO_HOA_ORDER>(hoaOrder);
    } else {
        MEDIA_LOG_W("Get channel layout failed");
    }
    format.Set<Tag::AUDIO_OBJECT_NUMBER>(objectNumber);
    format.Set<Tag::AUDIO_SOUNDBED_CHANNELS_NUMBER>(channels - objectNumber);
    // 设置一个整个音频内容通道总数
    format.Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(channelLayout);
    format.Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
    if (channels > 0) {
        format.Set<Tag::AUDIO_OUTPUT_CHANNELS>(static_cast<uint32_t>(channels));
        format.Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<uint32_t>(channels));
    } else {
        MEDIA_LOG_D("Parse channel count failed: " PUBLIC_LOG_D32, channels);
    }
}

void FFmpegFormatHelper::ParseTimedMetaTrackInfo(const AVStream& avStream, Meta &format)
{
    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avStream.metadata, "timed_metadata_key", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (valPtr == nullptr) {
        MEDIA_LOG_W("Get timed metadata key failed");
    } else {
        format.Set<Tag::TIMED_METADATA_KEY>(std::string(valPtr->value));
    }
    valPtr = av_dict_get(avStream.metadata, "src_track_id", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        MEDIA_LOG_W("Get src track id failed");
    } else {
        format.Set<Tag::TIMED_METADATA_SRC_TRACK>(std::stoi(valPtr->value));
    }
}

void FFmpegFormatHelper::ParseAuxiliaryTrackInfo(const AVStream& avStream, Meta &format)
{
    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avStream.metadata, "handler_name", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (valPtr == nullptr) {
        MEDIA_LOG_W("Get ref desc failed");
    } else {
        format.Set<Tag::TRACK_DESCRIPTION>(std::string(valPtr->value));
    }

    valPtr = av_dict_get(avStream.metadata, "track_reference_type", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (valPtr == nullptr) {
        MEDIA_LOG_W("Get ref type failed");
    } else {
        format.Set<Tag::TRACK_REFERENCE_TYPE>(std::string(valPtr->value));
    }

    valPtr = av_dict_get(avStream.metadata, "reference_track_ids", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        MEDIA_LOG_W("Get src track id failed");
    } else {
        std::vector<int32_t> referenceIds;
        std::vector<std::string> referenceIdsString;
        std::string values = std::string(valPtr->value);
        if (values.find(",") != std::string::npos) {
            referenceIdsString = SplitByChar(values.c_str(), ",");
        }
        FALSE_RETURN_MSG_W(referenceIdsString.size() > 0, "Auxl track without ref track");
        for (std::string subStr : referenceIdsString) {
            // Ffmpeg ensures that the value is definitely Int type
            int32_t id = -1;
            if (ConvertString<int32_t>(subStr, id)) {
                referenceIds.push_back(id);
            } else {
                MEDIA_LOG_W("error trackids " PUBLIC_LOG_S, subStr.c_str());
            }
        }
        format.Set<Tag::REFERENCE_TRACK_IDS>(referenceIds);
    }
}

void FFmpegFormatHelper::ParseHvccBoxInfo(const AVStream& avStream, Meta &format)
{
    HEVCProfile profile = FFMpegConverter::ConvertFFMpegToOHHEVCProfile(avStream.codecpar->profile);
    if (profile != HEVCProfile::HEVC_PROFILE_UNKNOW) {
        format.Set<Tag::VIDEO_H265_PROFILE>(profile);
        format.Set<Tag::MEDIA_PROFILE>(profile);
    } else {
        MEDIA_LOG_D("Parse hevc profile failed: " PUBLIC_LOG_D32, profile);
    }
    HEVCLevel level = FFMpegConverter::ConvertFFMpegToOHHEVCLevel(avStream.codecpar->level);
    if (level != HEVCLevel::HEVC_LEVEL_UNKNOW) {
        format.Set<Tag::VIDEO_H265_LEVEL>(level);
        format.Set<Tag::MEDIA_LEVEL>(level);
    } else {
        MEDIA_LOG_D("Parse hevc level failed: " PUBLIC_LOG_D32, level);
    }
}

void FFmpegFormatHelper::ParseColorBoxInfo(const AVStream& avStream, Meta &format)
{
    int colorRange = FFMpegConverter::ConvertFFMpegToOHColorRange(avStream.codecpar->color_range);
    format.Set<Tag::VIDEO_COLOR_RANGE>(static_cast<bool>(colorRange));

    ColorPrimary colorPrimaries = FFMpegConverter::ConvertFFMpegToOHColorPrimaries(avStream.codecpar->color_primaries);
    format.Set<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);

    TransferCharacteristic colorTrans = FFMpegConverter::ConvertFFMpegToOHColorTrans(avStream.codecpar->color_trc);
    format.Set<Tag::VIDEO_COLOR_TRC>(colorTrans);

    MatrixCoefficient colorMatrix = FFMpegConverter::ConvertFFMpegToOHColorMatrix(avStream.codecpar->color_space);
    format.Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix);

    ChromaLocation chromaLoc = FFMpegConverter::ConvertFFMpegToOHChromaLocation(avStream.codecpar->chroma_location);
    format.Set<Tag::VIDEO_CHROMA_LOCATION>(chromaLoc);
}

void FFmpegFormatHelper::ParseColorBoxInfo(const AVFormatContext& avFormatContext, HevcParseFormat parse, Meta &format)
{
    ColorPrimary colorPrimaries = FFMpegConverter::ConvertFFMpegToOHColorPrimaries(
        static_cast<AVColorPrimaries>(parse.colorPrimaries));
    if (colorPrimaries != ColorPrimary::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);
        format.Set<Tag::VIDEO_COLOR_RANGE>((bool)(parse.colorRange));
    } else {
        MEDIA_LOG_D("Parse colorPri from xps failed");
    }

    TransferCharacteristic colorTrans = FFMpegConverter::ConvertFFMpegToOHColorTrans(
        static_cast<AVColorTransferCharacteristic>(parse.colorTransfer));
    if (colorTrans != TransferCharacteristic::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_TRC>(colorTrans);
    } else {
        MEDIA_LOG_D("Parse colorTrc from xps failed");
    }

    MatrixCoefficient colorMatrix = FFMpegConverter::ConvertFFMpegToOHColorMatrix(
        static_cast<AVColorSpace>(parse.colorMatrixCoeff));
    if (colorMatrix != MatrixCoefficient::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix);
    } else {
        MEDIA_LOG_D("Parse colorMatrix from xps failed");
    }

    ChromaLocation chromaLoc = FFMpegConverter::ConvertFFMpegToOHChromaLocation(
        static_cast<AVChromaLocation>(parse.chromaLocation));
    if (chromaLoc != ChromaLocation::UNSPECIFIED) {
        format.Set<Tag::VIDEO_CHROMA_LOCATION>(chromaLoc);
    } else {
        MEDIA_LOG_D("Parse chromaLoc from xps failed");
    }
}

void FFmpegFormatHelper::ParseHevcInfo(const AVFormatContext &avFormatContext, HevcParseFormat parse, Meta &format)
{
    if (parse.isHdrVivid) {
        format.Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    }
    ParseColorBoxInfo(avFormatContext, parse, format);
    HEVCProfile profile = FFMpegConverter::ConvertFFMpegToOHHEVCProfile(static_cast<int>(parse.profile));
    if (profile != HEVCProfile::HEVC_PROFILE_UNKNOW) {
        format.Set<Tag::VIDEO_H265_PROFILE>(profile);
        format.Set<Tag::MEDIA_PROFILE>(profile);
    } else {
        MEDIA_LOG_D("Parse hevc profile failed: " PUBLIC_LOG_D32, profile);
    }
    HEVCLevel level = FFMpegConverter::ConvertFFMpegToOHHEVCLevel(static_cast<int>(parse.level));
    if (level != HEVCLevel::HEVC_LEVEL_UNKNOW) {
        format.Set<Tag::VIDEO_H265_LEVEL>(level);
        format.Set<Tag::MEDIA_LEVEL>(level);
    } else {
        MEDIA_LOG_D("Parse hevc level failed: " PUBLIC_LOG_D32, level);
    }
    auto FileType = GetFileTypeByName(avFormatContext);
    if (FileType == FileType::MPEGTS ||
        FileType == FileType::FLV) {
        format.Set<Tag::VIDEO_WIDTH>(static_cast<uint32_t>(parse.picWidInLumaSamples));
        format.Set<Tag::VIDEO_HEIGHT>(static_cast<uint32_t>(parse.picHetInLumaSamples));
    }
}

void FFmpegFormatHelper::ParseInfoFromMetadata(
    const AVDictionary* metadata, Meta &format, std::map<std::string, TagType> tagRange)
{
    AVDictionaryEntry *valPtr = nullptr;
    while ((valPtr = av_dict_get(metadata, "", valPtr, AV_DICT_IGNORE_SUFFIX)) != nullptr) {
        std::string tempKey = ToLower(std::string(valPtr->key));
        if (tempKey.find("moov_level_meta_key_") == 0) {
            MEDIA_LOG_D("UserMeta:" PUBLIC_LOG_S, valPtr->key);
            if (tagRange.count(tempKey.c_str() + KEY_PREFIX_LEN) > 0 &&
                strlen(valPtr->value) > VALUE_PREFIX_LEN) {
                    format.SetData(tagRange[tempKey.c_str() + KEY_PREFIX_LEN],
                                   std::string(valPtr->value + VALUE_PREFIX_LEN));
                }
            continue;
        }
        if (tagRange.count(tempKey) <= 0) {
            MEDIA_LOG_D("UnsupportMeta:" PUBLIC_LOG_S, valPtr->key);
            continue;
        }
        MEDIA_LOG_D("SupportMeta:" PUBLIC_LOG_S, valPtr->key);
        // ffmpeg use ';' to contact all single value in vorbis-comment, need to remove duplicates
        std::string value = RemoveDuplication(std::string(valPtr->value));
        format.SetData(tagRange[tempKey], value);
        AVDictionaryEntry *textEncodingPtr = nullptr;
        std::string keyEncoding = ToLower(std::string(valPtr->key)) + std::string("encoding");
        textEncodingPtr = av_dict_get(metadata, keyEncoding.c_str(), NULL, 0);
        if ((!IsUTF8(value.c_str()) && IsGBK(value.c_str())) ||
            (textEncodingPtr != nullptr && !strcmp(textEncodingPtr->value, "0"))) { // if encoding is 0, means GBK
            std::string resultStr = ConvertGBKToUTF8(value);
            if (resultStr.length() > 0) {
                format.SetData(tagRange[tempKey], resultStr);
            } else {
                MEDIA_LOG_D("Convert utf8 failed");
            }
        }
    }
}

bool FFmpegFormatHelper::IsVideoCodecId(const AVCodecID &codecId)
{
    if (g_codecIdToMime.count(codecId) == 0) {
        return false;
    }
    return StartWith(std::string(g_codecIdToMime[codecId]).c_str(), "video/");
}

bool FFmpegFormatHelper::IsImageTrack(const AVStream &avStream)
{
    FALSE_RETURN_V_MSG_E(avStream.codecpar != nullptr, false, "AVStream is nullptr");
    AVCodecID codecId = avStream.codecpar->codec_id;
    return (
        (static_cast<uint32_t>(avStream.disposition) & static_cast<uint32_t>(AV_DISPOSITION_ATTACHED_PIC)) ||
        (std::count(g_imageCodecID.begin(), g_imageCodecID.end(), codecId) > 0)
    );
}

bool FFmpegFormatHelper::IsVideoType(const AVStream &avStream)
{
    FALSE_RETURN_V_MSG_E(avStream.codecpar != nullptr, false, "AVStream is nullptr");
    AVCodecID codecId = avStream.codecpar->codec_id;
    return (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !IsImageTrack(avStream)) ||
        (g_codecIdToMime.count(codecId) > 0 && g_codecIdToMime[codecId].find("video") != std::string::npos);
}

bool FFmpegFormatHelper::IsAudioType(const AVStream &avStream)
{
    FALSE_RETURN_V_MSG_E(avStream.codecpar != nullptr, false, "AVStream is nullptr");
    AVCodecID codecId = avStream.codecpar->codec_id;
    return avStream.codecpar->codec_type == AVMEDIA_TYPE_AUDIO ||
        (g_codecIdToMime.count(codecId) > 0 && g_codecIdToMime[codecId].find("audio") != std::string::npos);
}

bool FFmpegFormatHelper::IsMpeg4File(FileType filetype)
{
    return filetype == FileType::MP4 || filetype == FileType::MOV
            || filetype == FileType::M4V || filetype == FileType::FT_3GP
            || filetype == FileType::FT_3G2;
}

bool FFmpegFormatHelper::IsValidCodecId(const AVCodecID &codecId)
{
    return (g_codecIdToMime.count(codecId) > 0) || (IsPCMStream(codecId));
}

void FFmpegFormatHelper::ParseGltfInfo(const AVFormatContext& avFormatContext, Meta &format)
{
    FALSE_RETURN_MSG_W(avFormatContext.metadata != nullptr, "metadata is nullptr");
    auto compatibleBrands = av_dict_get(avFormatContext.metadata, "compatible_brands", NULL, 0);
    if (compatibleBrands != nullptr && compatibleBrands->value != nullptr) {
        const char* value = compatibleBrands->value;
        size_t len = strlen(value);
        for (size_t i = 0; i + BRAND_CODE_LEN <= len; i += BRAND_CODE_LEN) {
            if (strncmp(value + i, "glti", BRAND_CODE_LEN) == 0) {
                format.Set<Tag::IS_GLTF>(true);
                break;
            }
        }
    }
    for (const char* key : g_gltfKeys) {
        auto entry = av_dict_get(avFormatContext.metadata, key, NULL, 0);
        if (!entry || strcmp(entry->value, "1") != 0) {
            return;
        }
    }
    auto idatPos = av_dict_get(avFormatContext.metadata, "idat_pos", NULL, 0);
    MEDIA_LOG_I("Valid glTF file and idat Pos: " PUBLIC_LOG_S, idatPos ? idatPos->value : "not found");
    if (idatPos && idatPos->value) {
        SetToFormatIfConvertSuccess(format, Tag::GLTF_OFFSET, idatPos->value, ValueType::INT64);
    }
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS