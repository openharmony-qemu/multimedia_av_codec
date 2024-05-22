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

#define HST_LOG_TAG "FfmpegFormatHelper"

#include <algorithm>
#include <iconv.h>
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

#define CUVA_VERSION_MAP (static_cast<uint16_t>(1))
#define TERMINAL_PROVIDE_CODE (static_cast<uint16_t>(4))
#define TERMINAL_PROVIDE_ORIENTED_CODE (static_cast<uint16_t>(5))

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const uint32_t MAX_VALUE_LEN = 256;
const uint32_t DOUBLE_BYTES = 2;
const uint32_t KEY_PREFIX_LEN = 20;
const uint32_t VALUE_PREFIX_LEN = 8;
const uint32_t VALID_LOCATION_LEN = 2;
namespace {
static std::map<AVMediaType, MediaType> g_convertFfmpegTrackType = {
    {AVMEDIA_TYPE_VIDEO, MediaType::VIDEO},
    {AVMEDIA_TYPE_AUDIO, MediaType::AUDIO},
    {AVMEDIA_TYPE_SUBTITLE, MediaType::SUBTITLE},
};

static std::map<AVCodecID, std::string_view> g_codecIdToMime = {
    {AV_CODEC_ID_MP1, MimeType::AUDIO_MPEG},
    {AV_CODEC_ID_MP2, MimeType::AUDIO_MPEG},
    {AV_CODEC_ID_MP3, MimeType::AUDIO_MPEG},
    {AV_CODEC_ID_FLAC, MimeType::AUDIO_FLAC},
    {AV_CODEC_ID_AAC, MimeType::AUDIO_AAC},
    {AV_CODEC_ID_VORBIS, MimeType::AUDIO_VORBIS},
    {AV_CODEC_ID_OPUS, MimeType::AUDIO_OPUS},
    {AV_CODEC_ID_AMR_NB, MimeType::AUDIO_AMR_NB},
    {AV_CODEC_ID_AMR_WB, MimeType::AUDIO_AMR_WB},
    {AV_CODEC_ID_H264, MimeType::VIDEO_AVC},
    {AV_CODEC_ID_MPEG4, MimeType::VIDEO_MPEG4},
    {AV_CODEC_ID_MJPEG, MimeType::IMAGE_JPG},
    {AV_CODEC_ID_PNG, MimeType::IMAGE_PNG},
    {AV_CODEC_ID_BMP, MimeType::IMAGE_BMP},
    {AV_CODEC_ID_H263, MimeType::VIDEO_H263},
    {AV_CODEC_ID_MPEG2TS, MimeType::VIDEO_MPEG2},
    {AV_CODEC_ID_MPEG2VIDEO, MimeType::VIDEO_MPEG2},
    {AV_CODEC_ID_HEVC, MimeType::VIDEO_HEVC},
    {AV_CODEC_ID_VP8, MimeType::VIDEO_VP8},
    {AV_CODEC_ID_VP9, MimeType::VIDEO_VP9},
    {AV_CODEC_ID_AVS3DA, MimeType::AUDIO_AVS3DA},
    {AV_CODEC_ID_PCM_MULAW, MimeType::AUDIO_G711MU},
    {AV_CODEC_ID_APE, MimeType::AUDIO_APE},
    {AV_CODEC_ID_SUBRIP, MimeType::TEXT_SUBRIP}
};

static std::map<std::string, FileType> g_convertFfmpegFileType = {
    {"mpegts", FileType::MPEGTS},
    {"matroska,webm", FileType::MKV},
    {"amr", FileType::AMR},
    {"aac", FileType::AAC},
    {"mp3", FileType::MP3},
    {"flac", FileType::FLAC},
    {"ogg", FileType::OGG},
    {"wav", FileType::WAV},
    {"flv", FileType::FLV},
    {"ape", FileType::APE},
    {"srt", FileType::SRT},
};

static std::map<TagType, std::string> g_formatToString = {
    {Tag::MEDIA_TITLE, "title"},
    {Tag::MEDIA_ARTIST, "artist"},
    {Tag::MEDIA_ALBUM, "album"},
    {Tag::MEDIA_ALBUM_ARTIST, "album_artist"},
    {Tag::MEDIA_DATE, "date"},
    {Tag::MEDIA_COMMENT, "comment"},
    {Tag::MEDIA_GENRE, "genre"},
    {Tag::MEDIA_COPYRIGHT, "copyright"},
    {Tag::MEDIA_LANGUAGE, "language"},
    {Tag::MEDIA_DESCRIPTION, "description"},
    {Tag::MEDIA_LYRICS, "lyrics"},
    {Tag::MEDIA_AUTHOR, "author"},
    {Tag::MEDIA_COMPOSER, "composer"},
    {Tag::MEDIA_CREATION_TIME, "creation_time"}
};

static std::vector<TagType> g_supportSourceFormat = {
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
    Tag::MEDIA_CREATION_TIME
};

std::string SwitchCase(const std::string& str)
{
    std::string res;
    for (char c : str) {
        if (c == '_') {
            res += c;
        } else {
            res += std::toupper(c);
        }
    }
    MEDIA_LOG_W("Parse meta " PUBLIC_LOG_S " failed, try to parse " PUBLIC_LOG_S "", str.c_str(), res.c_str());
    return res;
}

int ConvertGBK2UTF8(char* input, const size_t inputLen, char* output, const size_t outputLen)
{
    MEDIA_LOG_D("Convert GBK to UTF-8, inputLen=" PUBLIC_LOG_ZU, inputLen);
    int resultLen = -1;
    size_t inputTempLen = inputLen;
    size_t outputTempLen = outputLen;
    iconv_t cd = iconv_open("UTF-8", "GB2312");
    if (cd != reinterpret_cast<iconv_t>(-1)) {
        size_t ret = iconv(cd, &input, &inputTempLen, &output, &outputTempLen);
        if (ret != static_cast<size_t>(-1))  {
            resultLen = static_cast<int>(outputLen - outputTempLen);
        } else {
            MEDIA_LOG_D("Convert failed");
        }
        iconv_close(cd);
    }
    MEDIA_LOG_D("Convert GBK to UTF-8, resultLen=" PUBLIC_LOG_D32, resultLen);
    return resultLen;
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
    AV_CODEC_ID_MJPEG,
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

bool IsPCMStream(AVCodecID codecID)
{
    MEDIA_LOG_D("CodecID " PUBLIC_LOG_D32 "[" PUBLIC_LOG_S "].",
        static_cast<int32_t>(codecID), avcodec_get_name(codecID));
    return StartWith(avcodec_get_name(codecID), "pcm_");
}
} // namespace

void FFmpegFormatHelper::ParseMediaInfo(const AVFormatContext& avFormatContext, Meta& format)
{
    format.Set<Tag::MEDIA_TRACK_COUNT>(static_cast<int32_t>(avFormatContext.nb_streams));
    bool hasVideo = false;
    bool hasAudio = false;
    bool hasSubtitle = false;
    for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
        if (avFormatContext.streams[i] == nullptr || avFormatContext.streams[i]->codecpar == nullptr) {
            MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " is invalid.", i);
            continue;
        }
        if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodecID codecID = avFormatContext.streams[i]->codecpar->codec_id;
            if (std::count(g_imageCodecID.begin(), g_imageCodecID.end(), codecID) <= 0) {
                hasVideo = true;
            }
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            hasAudio = true;
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            hasSubtitle = true;
        }
    }
    format.Set<Tag::MEDIA_HAS_VIDEO>(hasVideo);
    format.Set<Tag::MEDIA_HAS_AUDIO>(hasAudio);
    format.Set<Tag::MEDIA_HAS_SUBTITLE>(hasSubtitle);
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
    if (duration <= 0) {
        for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
            if (avFormatContext.streams[i]->duration > duration) {
                duration = avFormatContext.streams[i]->duration;
            }
        }
    } else {
        format.Set<Tag::MEDIA_DURATION>(static_cast<int64_t>(duration));
    }
    ParseLocationInfo(avFormatContext, format);
    for (TagType key: g_supportSourceFormat) {
        ParseInfoFromMetadata(avFormatContext.metadata, key, format);
    }
}

std::vector<std::string> SplitByChar(const char* str, const char* pattern)
{
    std::vector<std::string> resultVec;
    char* tmpStr = strtok(const_cast<char*>(str), pattern);
    while (tmpStr != nullptr) {
        resultVec.push_back(std::string(tmpStr));
        tmpStr = strtok(nullptr,  pattern);
    }
    MEDIA_LOG_D("Split [" PUBLIC_LOG_S "] by [" PUBLIC_LOG_S "], get " PUBLIC_LOG_ZU " string",
        str, pattern, resultVec.size());
    delete[] tmpStr;
    return resultVec;
}

void FFmpegFormatHelper::ParseLocationInfo(const AVFormatContext& avFormatContext, Meta &format)
{
    MEDIA_LOG_D("Parse location info.");
    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avFormatContext.metadata, "location", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        valPtr = av_dict_get(avFormatContext.metadata, "LOCATION", nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        MEDIA_LOG_D("Parse failed.");
        return;
    }
    MEDIA_LOG_D("Parse location info successfully: " PUBLIC_LOG_S, valPtr->value);
    std::vector<std::string> values = SplitByChar(valPtr->value, "+");
    if (values.size() < VALID_LOCATION_LEN) {
        MEDIA_LOG_D("Parse failed due to info format error.");
        return;
    }

    format.Set<Tag::MEDIA_LATITUDE>(std::stof(values[0]));
    if (values[1].find('/') != 0) {
        format.Set<Tag::MEDIA_LONGITUDE>(std::stof(SplitByChar(values[1].c_str(), "/")[0]));
    } else {
        format.Set<Tag::MEDIA_LONGITUDE>(std::stof(values[1]));
    }
}

void FFmpegFormatHelper::ParseUserMeta(const AVFormatContext& avFormatContext, std::shared_ptr<Meta> format)
{
    MEDIA_LOG_D("Parse user data info.");
    AVDictionaryEntry *valPtr = nullptr;
    while ((valPtr = av_dict_get(avFormatContext.metadata, "", valPtr, AV_DICT_IGNORE_SUFFIX)))  {
        if (StartWith(valPtr->key, "moov_level_meta_key_")) {
            MEDIA_LOG_D("ffmpeg key: " PUBLIC_LOG_S, (valPtr->key));
            if (strlen(valPtr->value) <= VALUE_PREFIX_LEN) {
                MEDIA_LOG_D("Parse user data info " PUBLIC_LOG_S " failed, value too short.", valPtr->key);
                continue;
            }
            if (StartWith(valPtr->value, "00000001")) { // string
                MEDIA_LOG_D("key: " PUBLIC_LOG_S " | type: string", (valPtr->key + KEY_PREFIX_LEN));
                format->SetData(valPtr->key + KEY_PREFIX_LEN, std::string(valPtr->value + VALUE_PREFIX_LEN));
            } else if (StartWith(valPtr->value, "00000017")) { // float
                MEDIA_LOG_D("key: " PUBLIC_LOG_S " | type: float", (valPtr->key + KEY_PREFIX_LEN));
                format->SetData(valPtr->key + KEY_PREFIX_LEN, std::stof(valPtr->value + VALUE_PREFIX_LEN));
            } else if (StartWith(valPtr->value, "00000043") || StartWith(valPtr->value, "00000015")) { // int
                MEDIA_LOG_D("key: " PUBLIC_LOG_S " | type: int", (valPtr->key + KEY_PREFIX_LEN));
                format->SetData(valPtr->key + KEY_PREFIX_LEN, std::stoi(valPtr->value + VALUE_PREFIX_LEN));
            } else { // unknow
                MEDIA_LOG_D("key: " PUBLIC_LOG_S " | type: unknow", (valPtr->key + KEY_PREFIX_LEN));
                format->SetData(valPtr->key + KEY_PREFIX_LEN, std::string(valPtr->value + VALUE_PREFIX_LEN));
            }
        }
    }
}

void FFmpegFormatHelper::ParseTrackInfo(const AVStream& avStream, Meta& format)
{
    FALSE_RETURN_MSG(avStream.codecpar != nullptr, "Parse track info failed due to codec par is nullptr.");
    ParseBaseTrackInfo(avStream, format);
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if ((static_cast<uint32_t>(avStream.disposition) & static_cast<uint32_t>(AV_DISPOSITION_ATTACHED_PIC)) ||
            (std::count(g_imageCodecID.begin(), g_imageCodecID.end(), avStream.codecpar->codec_id) > 0)) {
            ParseImageTrackInfo(avStream, format);
        } else {
            ParseAVTrackInfo(avStream, format);
            ParseVideoTrackInfo(avStream, format);
        }
    } else if (avStream.codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        ParseAVTrackInfo(avStream, format);
        ParseAudioTrackInfo(avStream, format);
    }
}

void FFmpegFormatHelper::ParseBaseTrackInfo(const AVStream& avStream, Meta &format)
{
    if (g_codecIdToMime.count(avStream.codecpar->codec_id) != 0) {
        format.Set<Tag::MIME_TYPE>(std::string(g_codecIdToMime[avStream.codecpar->codec_id]));
    } else if (IsPCMStream(avStream.codecpar->codec_id)) {
        format.Set<Tag::MIME_TYPE>(std::string(MimeType::AUDIO_RAW));
    } else {
        MEDIA_LOG_D("Parse mime type info failed: " PUBLIC_LOG_D32 ".",
            static_cast<int32_t>(avStream.codecpar->codec_id));
    }

    AVMediaType mediaType = avStream.codecpar->codec_type;
    if (g_convertFfmpegTrackType.count(mediaType) > 0) {
        format.Set<Tag::MEDIA_TYPE>(g_convertFfmpegTrackType[mediaType]);
    } else {
        MEDIA_LOG_D("Parse track type info failed: " PUBLIC_LOG_D32 ".",
            static_cast<int32_t>(avStream.codecpar->codec_type));
    }
}

FileType FFmpegFormatHelper::GetFileTypeByName(const AVFormatContext& avFormatContext)
{
    FALSE_RETURN_V_MSG_E(avFormatContext.iformat != nullptr, FileType::UNKNOW, "iformat is nullptr.");
    const char *fileName = avFormatContext.iformat->name;
    FileType fileType = FileType::UNKNOW;
    if (StartWith(fileName, "mov,mp4,m4a")) {
        const AVDictionaryEntry *type = av_dict_get(avFormatContext.metadata, "major_brand", NULL, 0);
        if (type == nullptr) {
            return FileType::UNKNOW;
        }
        if (StartWith(type->value, "m4a") || StartWith(type->value, "M4A") ||
            StartWith(type->value, "m4v") || StartWith(type->value, "M4V")) {
            fileType = FileType::M4A;
        } else {
            fileType = FileType::MP4;
        }
    } else {
        if (g_convertFfmpegFileType.count(fileName) != 0) {
            fileType = g_convertFfmpegFileType[fileName];
        }
    }
    MEDIA_LOG_D("file name [" PUBLIC_LOG_S "] file type [" PUBLIC_LOG_D32 "].",
        fileName, static_cast<int32_t>(fileType));
    return fileType;
}

void FFmpegFormatHelper::ParseAVTrackInfo(const AVStream& avStream, Meta &format)
{
    int64_t bitRate = static_cast<int64_t>(avStream.codecpar->bit_rate);
    if (bitRate > 0) {
        format.Set<Tag::MEDIA_BITRATE>(bitRate);
    } else {
        MEDIA_LOG_D("Parse bitrate info failed: ." PUBLIC_LOG_D64, bitRate);
    }

    if (avStream.codecpar->extradata_size > 0 && avStream.codecpar->extradata != nullptr) {
        std::vector<uint8_t> extra(avStream.codecpar->extradata_size);
        extra.assign(avStream.codecpar->extradata, avStream.codecpar->extradata + avStream.codecpar->extradata_size);
        format.Set<Tag::MEDIA_CODEC_CONFIG>(extra);
    } else {
        MEDIA_LOG_D("Parse codec config info failed.");
    }
    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avStream.metadata, "language", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr != nullptr) {
        format.SetData(Tag::MEDIA_LANGUAGE, std::string(valPtr->value));
    } else {
        MEDIA_LOG_D("Parse track language info failed.");
    }
    format.SetData(Tag::MEDIA_START_TIME,
        ConvertTimeFromFFmpeg(avStream.start_time, avStream.time_base));
}

void FFmpegFormatHelper::ParseVideoTrackInfo(const AVStream& avStream, Meta &format)
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
        MEDIA_LOG_D("Parse frame rate info failed: " PUBLIC_LOG_F ".", frameRate);
    }

    AVDictionaryEntry *valPtr = nullptr;
    valPtr = av_dict_get(avStream.metadata, "rotate", nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        valPtr = av_dict_get(avStream.metadata, "ROTATE", nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        MEDIA_LOG_D("Parse rotate info from meta failed.");
        ParseRotationFromMatrix(avStream, format);
    } else {
        if (g_pFfRotationMap.count(std::string(valPtr->value)) > 0) {
            format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap[std::string(valPtr->value)]);
        }
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
            rotation += 360;
        }
        switch (int(rotation)) {
            case 90:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["90"]);
                break;
            case 180:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["180"]);
                break;
            case 270:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["270"]);
                break;
            default:
                format.Set<Tag::VIDEO_ROTATION>(g_pFfRotationMap["0"]);
                break;
        }
    }
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
        MEDIA_LOG_D("Parse cover info failed: " PUBLIC_LOG_D32 ".", pkt.size);
    }
}

void FFmpegFormatHelper::ParseAudioTrackInfo(const AVStream& avStream, Meta &format)
{
    int sampelRate = avStream.codecpar->sample_rate;
    int channels = avStream.codecpar->channels;
    int frameSize = avStream.codecpar->frame_size;
    if (sampelRate > 0) {
        format.Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<uint32_t>(sampelRate));
    } else {
        MEDIA_LOG_D("Parse sample rate info failed: " PUBLIC_LOG_D32 ".", sampelRate);
    }
    if (channels > 0) {
        format.Set<Tag::AUDIO_OUTPUT_CHANNELS>(static_cast<uint32_t>(channels));
        format.Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<uint32_t>(channels));
    } else {
        MEDIA_LOG_D("Parse channel count info failed: " PUBLIC_LOG_D32 ".", channels);
    }
    if (frameSize > 0) {
        format.Set<Tag::AUDIO_SAMPLE_PER_FRAME>(static_cast<uint32_t>(frameSize));
    } else {
        MEDIA_LOG_D("Parse frame rate info failed: " PUBLIC_LOG_D32 ".", frameSize);
    }
    AudioChannelLayout channelLayout = FFMpegConverter::ConvertFFToOHAudioChannelLayoutV2(
        avStream.codecpar->channel_layout, channels);
    format.Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(channelLayout);
    format.Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
    
    AudioSampleFormat fmt;
    if (!IsPCMStream(avStream.codecpar->codec_id)) {
        fmt = FFMpegConverter::ConvertFFMpegToOHAudioFormat(static_cast<AVSampleFormat>(avStream.codecpar->format));
    } else {
        fmt = FFMpegConverter::ConvertFFMpegAVCodecIdToOHAudioFormat(avStream.codecpar->codec_id);
    }
    format.Set<Tag::AUDIO_SAMPLE_FORMAT>(fmt);

    if (avStream.codecpar->codec_id == AV_CODEC_ID_AAC) {
        format.Set<Tag::AUDIO_AAC_IS_ADTS>(1);
    } else if (avStream.codecpar->codec_id == AV_CODEC_ID_AAC_LATM) {
        format.Set<Tag::AUDIO_AAC_IS_ADTS>(0);
    }
    format.Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(avStream.codecpar->bits_per_coded_sample);
}

void FFmpegFormatHelper::ParseHvccBoxInfo(const AVStream& avStream, Meta &format)
{
    HEVCProfile profile = FFMpegConverter::ConvertFFMpegToOHHEVCProfile(avStream.codecpar->profile);
    if (profile != HEVCProfile::HEVC_PROFILE_UNKNOW) {
        format.Set<Tag::VIDEO_H265_PROFILE>(profile);
        format.Set<Tag::MEDIA_PROFILE>(profile);
    } else {
        MEDIA_LOG_D("Parse hevc profile info failed: " PUBLIC_LOG_D32 ".", profile);
    }
    HEVCLevel level = FFMpegConverter::ConvertFFMpegToOHHEVCLevel(avStream.codecpar->level);
    if (level != HEVCLevel::HEVC_LEVEL_UNKNOW) {
        format.Set<Tag::VIDEO_H265_LEVEL>(level);
        format.Set<Tag::MEDIA_LEVEL>(level);
    } else {
        MEDIA_LOG_D("Parse hevc level info failed: " PUBLIC_LOG_D32 ".", level);
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

void FFmpegFormatHelper::ParseHevcInfo(const AVFormatContext &avFormatContext, HevcParseFormat parse, Meta &format)
{
    if (parse.isHdrVivid) {
        format.Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    }

    format.Set<Tag::VIDEO_COLOR_RANGE>((bool)(parse.colorRange));

    ColorPrimary colorPrimaries = FFMpegConverter::ConvertFFMpegToOHColorPrimaries(
        static_cast<AVColorPrimaries>(parse.colorPrimaries));
    format.Set<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);

    TransferCharacteristic colorTrans = FFMpegConverter::ConvertFFMpegToOHColorTrans(
        static_cast<AVColorTransferCharacteristic>(parse.colorTransfer));
    format.Set<Tag::VIDEO_COLOR_TRC>(colorTrans);

    MatrixCoefficient colorMatrix = FFMpegConverter::ConvertFFMpegToOHColorMatrix(
        static_cast<AVColorSpace>(parse.colorMatrixCoeff));
    format.Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix);

    ChromaLocation chromaLoc = FFMpegConverter::ConvertFFMpegToOHChromaLocation(
        static_cast<AVChromaLocation>(parse.chromaLocation));
    format.Set<Tag::VIDEO_CHROMA_LOCATION>(chromaLoc);

    HEVCProfile profile = FFMpegConverter::ConvertFFMpegToOHHEVCProfile(static_cast<int>(parse.profile));
    if (profile != HEVCProfile::HEVC_PROFILE_UNKNOW) {
        format.Set<Tag::VIDEO_H265_PROFILE>(profile);
        format.Set<Tag::MEDIA_PROFILE>(profile);
    } else {
        MEDIA_LOG_D("Parse hevc profile info failed: " PUBLIC_LOG_D32 ".", profile);
    }
    HEVCLevel level = FFMpegConverter::ConvertFFMpegToOHHEVCLevel(static_cast<int>(parse.level));
    if (level != HEVCLevel::HEVC_LEVEL_UNKNOW) {
        format.Set<Tag::VIDEO_H265_LEVEL>(level);
        format.Set<Tag::MEDIA_LEVEL>(level);
    } else {
        MEDIA_LOG_D("Parse hevc level info failed: " PUBLIC_LOG_D32 ".", level);
    }

    if (GetFileTypeByName(avFormatContext) == FileType::MPEGTS ||
        GetFileTypeByName(avFormatContext) == FileType::FLV) {
        MEDIA_LOG_I("Updata info for mpegts from parser");
        format.Set<Tag::VIDEO_WIDTH>(static_cast<uint32_t>(parse.picWidInLumaSamples));
        format.Set<Tag::VIDEO_HEIGHT>(static_cast<uint32_t>(parse.picHetInLumaSamples));
    }
}

void FFmpegFormatHelper::ParseInfoFromMetadata(const AVDictionary* metadata, const TagType key, Meta &format)
{
    MEDIA_LOG_D("Parse " PUBLIC_LOG_S " info.", key.c_str());
    AVDictionaryEntry *valPtr = nullptr;
    bool parseFromMoov = false;
    valPtr = av_dict_get(metadata, g_formatToString[key].c_str(), nullptr, AV_DICT_MATCH_CASE);
    if (valPtr == nullptr) {
        valPtr = av_dict_get(metadata, SwitchCase(std::string(key)).c_str(), nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        valPtr = av_dict_get(metadata, ("moov_level_meta_key_" + std::string(key)).c_str(),
            nullptr, AV_DICT_MATCH_CASE);
        parseFromMoov = true;
    }
    if (valPtr == nullptr) {
        MEDIA_LOG_D("Parse failed");
        return;
    }
    if (parseFromMoov) {
        if (strlen(valPtr->value) > VALUE_PREFIX_LEN) {
            format.SetData(key, std::string(valPtr->value + VALUE_PREFIX_LEN));
        }
        return;
    }
    format.SetData(key, std::string(valPtr->value));
    if (IsGBK(valPtr->value)) {
        int inputLen = strlen(valPtr->value);
        char* utf8Result = new char[MAX_VALUE_LEN + 1];
        utf8Result[MAX_VALUE_LEN] = '\0';
        int resultLen = ConvertGBK2UTF8(valPtr->value, inputLen, utf8Result, MAX_VALUE_LEN);
        if (resultLen >= 0) { // In some case, utf8Result will contains extra characters, extract the valid parts
            char *subStr = new char[resultLen + 1];
            int ret = memcpy_s(subStr, resultLen, utf8Result, resultLen);
            if (ret == EOK) {
                subStr[resultLen] = '\0';
                format.SetData(key, std::string(subStr));
            }
            delete[] subStr;
        }
        delete[] utf8Result;
    }
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS