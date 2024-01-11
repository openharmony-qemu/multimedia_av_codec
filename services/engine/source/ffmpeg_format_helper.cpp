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

#include "ffmpeg_format_helper.h"
#include "media_description.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_info.h"
#include "ffmpeg_converter.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avutil.h"
#include "libavutil/mastering_display_metadata.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FFmpegFormatHelper"};

    static std::map<AVMediaType, MediaType> g_convertFfmpegTrackType = {
        {AVMEDIA_TYPE_VIDEO, MediaType::MEDIA_TYPE_VID},
        {AVMEDIA_TYPE_AUDIO, MediaType::MEDIA_TYPE_AUD},
    };

    static std::map<AVCodecID, std::string_view> g_codecIdToMime = {
        {AV_CODEC_ID_MP3, CodecMimeType::AUDIO_MPEG},
        {AV_CODEC_ID_FLAC, CodecMimeType::AUDIO_FLAC},
        {AV_CODEC_ID_AAC, CodecMimeType::AUDIO_AAC},
        {AV_CODEC_ID_VORBIS, CodecMimeType::AUDIO_VORBIS},
        {AV_CODEC_ID_OPUS, CodecMimeType::AUDIO_OPUS},
        {AV_CODEC_ID_AMR_NB, CodecMimeType::AUDIO_AMR_NB},
        {AV_CODEC_ID_AMR_WB, CodecMimeType::AUDIO_AMR_WB},
        {AV_CODEC_ID_H264, CodecMimeType::VIDEO_AVC},
        {AV_CODEC_ID_MPEG4, CodecMimeType::VIDEO_MPEG4},
        {AV_CODEC_ID_MJPEG, CodecMimeType::IMAGE_JPG},
        {AV_CODEC_ID_PNG, CodecMimeType::IMAGE_PNG},
        {AV_CODEC_ID_BMP, CodecMimeType::IMAGE_BMP},
        {AV_CODEC_ID_H263, CodecMimeType::VIDEO_H263},
        {AV_CODEC_ID_MPEG2TS, CodecMimeType::VIDEO_MPEG2},
        {AV_CODEC_ID_MPEG2VIDEO, CodecMimeType::VIDEO_MPEG2},
        {AV_CODEC_ID_HEVC, CodecMimeType::VIDEO_HEVC},
        {AV_CODEC_ID_VP8, CodecMimeType::VIDEO_VP8},
        {AV_CODEC_ID_VP9, CodecMimeType::VIDEO_VP9},
        {AV_CODEC_ID_AVS3DA, CodecMimeType::AUDIO_AVS3DA},
    };

    static std::map<std::string, FileType> g_convertFfmpegFileType = {
        {"mpegts", FileType::FILE_TYPE_MPEGTS},
        {"matroska,webm", FileType::FILE_TYPE_MKV},
        {"amr", FileType::FILE_TYPE_AMR},
        {"aac", FileType::FILE_TYPE_AAC},
        {"mp3", FileType::FILE_TYPE_MP3},
        {"flac", FileType::FILE_TYPE_FLAC},
        {"ogg", FileType::FILE_TYPE_OGG},
        {"wav", FileType::FILE_TYPE_WAV},
    };

    static std::map<std::string_view, std::string> g_formatToString = {
        {MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, "rotate"},
    };

    static std::vector<std::string_view> g_supportSourceFormat = {
        AVSourceFormat::SOURCE_TITLE,
        AVSourceFormat::SOURCE_ARTIST,
        AVSourceFormat::SOURCE_ALBUM,
        AVSourceFormat::SOURCE_ALBUM_ARTIST,
        AVSourceFormat::SOURCE_DATE,
        AVSourceFormat::SOURCE_COMMENT,
        AVSourceFormat::SOURCE_GENRE,
        AVSourceFormat::SOURCE_COPYRIGHT,
        AVSourceFormat::SOURCE_LANGUAGE,
        AVSourceFormat::SOURCE_DESCRIPTION,
        AVSourceFormat::SOURCE_LYRICS,
        AVSourceFormat::SOURCE_AUTHOR,
        AVSourceFormat::SOURCE_COMPOSER,
    };

    std::string SwitchCase(std::string str)
    {
        std::string res;
        for (char c : str) {
            if (c == '_') {
                res += c;
            } else {
                res += std::toupper(c);
            }
        }
        AVCODEC_LOGW("Parse meta %{public}s failed, try to parse %{public}s", str.c_str(), res.c_str());
        return res;
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

    bool StartWith(const char* name, const char* chars)
    {
        return !strncmp(name, chars, strlen(chars));
    }

    bool IsPCMStream(AVCodecID codecID)
    {
        return StartWith(avcodec_get_name(codecID), "pcm_");
    }

    FileType GetFileTypeByName(const AVFormatContext& avFormatContext)
    {
        const char *fileName = avFormatContext.iformat->name;
        FileType fileType = FileType::FILE_TYPE_UNKNOW;
        if (StartWith(fileName, "mov,mp4,m4a")) {
            fileType = FileType::FILE_TYPE_MP4;
            const AVDictionaryEntry *type = av_dict_get(avFormatContext.metadata, "major_brand", NULL, 0);
            if (type != nullptr && (StartWith(type->value, "m4a") || StartWith(type->value, "M4A"))) {
                fileType = FileType::FILE_TYPE_M4A;
            }
        } else {
            if (g_convertFfmpegFileType.count(fileName) != 0) {
                fileType = g_convertFfmpegFileType[fileName];
            }
        }
        return fileType;
    }
}

void FFmpegFormatHelper::ParseMediaInfo(const AVFormatContext& avFormatContext, Format &format)
{
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_TRACK_COUNT, static_cast<int32_t>(avFormatContext.nb_streams), format);

    bool hasVideo = false;
    bool hasAudio = false;
    for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
        if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodecID codecID = avFormatContext.streams[i]->codecpar->codec_id;
            if (std::count(g_imageCodecID.begin(), g_imageCodecID.end(), codecID) <= 0) {
                hasVideo = true;
            }
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            hasAudio = true;
        }
    }
    PutInfoToFormat(AVSourceFormat::SOURCE_HAS_VIDEO, static_cast<int32_t>(hasVideo), format);
    PutInfoToFormat(AVSourceFormat::SOURCE_HAS_AUDIO, static_cast<int32_t>(hasAudio), format);

    PutInfoToFormat(AVSourceFormat::SOURCE_FILE_TYPE,
        static_cast<int32_t>(GetFileTypeByName(avFormatContext)), format);
    
    int64_t duration = avFormatContext.duration;
    if (duration == AV_NOPTS_VALUE) {
        duration = 0;
        const AVDictionaryEntry *metaDuration = av_dict_get(avFormatContext.metadata, "DURATION", NULL, 0);
        int64_t us;
        if (metaDuration && (av_parse_time(&us, metaDuration->value, 1) == 0)) {
            if (us > duration) {
                duration = us;
            }
        }
    }
    if (duration <= 0) {
        for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
            auto streamDuration = avFormatContext.streams[i]->duration;
            if (streamDuration > duration) {
                duration = streamDuration;
            }
        }
    }
    if (duration > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_DURATION, static_cast<int64_t>(duration), format);
    } else {
        AVCODEC_LOGW("Parse duration info failed: %{public}" PRId64, duration);
    }

    for (std::string_view key: g_supportSourceFormat) {
        ParseInfoFromMetadata(avFormatContext.metadata, key, format);
    }
}

void FFmpegFormatHelper::ParseTrackInfo(const AVStream& avStream, Format &format)
{
    ParseBaseTrackInfo(avStream, format);
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if ((avStream.disposition & AV_DISPOSITION_ATTACHED_PIC) ||
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

void FFmpegFormatHelper::ParseHevcInfo(const AVFormatContext &avFormatContext, HevcParseFormat parse, Format &format)
{
    if (parse.isHdrVivid) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID, parse.isHdrVivid, format);
    }

    PutInfoToFormat(MediaDescriptionKey::MD_KEY_RANGE_FLAG, parse.colorRange, format);

    int32_t colorPrimaries = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHColorPrimaries(
        static_cast<AVColorPrimaries>(parse.colorPrimaries)));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, colorPrimaries, format);

    int32_t colorTransfer = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHColorTrans(
        static_cast<AVColorTransferCharacteristic>(parse.colorTransfer)));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, colorTransfer, format);

    int32_t colorMatrixCoeff = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHColorMatrix(
        static_cast<AVColorSpace>(parse.colorMatrixCoeff)));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, colorMatrixCoeff, format);

    int32_t profile = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHHEVCProfile(
        static_cast<int>(parse.profile)));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_PROFILE, profile, format);

    int32_t level = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHHEVCLevel(
        static_cast<int>(parse.level)));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_LEVEL, level, format);

    int32_t chromaLocation = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHChromaLocation(
        static_cast<AVChromaLocation>(parse.chromaLocation)));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_CHROMA_LOCATION, chromaLocation, format);

    if (GetFileTypeByName(avFormatContext) == FileType::FILE_TYPE_MPEGTS) {
        AVCODEC_LOGI("Updata info for mpegts from parser");
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_WIDTH, static_cast<int32_t>(parse.picWidInLumaSamples), format);
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_HEIGHT, static_cast<int32_t>(parse.picHetInLumaSamples), format);
    }
}

void FFmpegFormatHelper::ParseBaseTrackInfo(const AVStream& avStream, Format &format)
{
    if (g_codecIdToMime.count(avStream.codecpar->codec_id) != 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_CODEC_MIME, g_codecIdToMime[avStream.codecpar->codec_id], format);
    } else if (IsPCMStream(avStream.codecpar->codec_id)) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_RAW, format);
    } else {
        AVCODEC_LOGW("Parse mime type info failed: %{public}d", static_cast<int32_t>(avStream.codecpar->codec_id));
    }

    AVMediaType mediaType = avStream.codecpar->codec_type;
    if (g_convertFfmpegTrackType.count(mediaType) > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_TRACK_TYPE, g_convertFfmpegTrackType[mediaType], format);
    } else {
        AVCODEC_LOGW("Parse track type info failed: %{public}d", static_cast<int32_t>(avStream.codecpar->codec_type));
    }
}

void FFmpegFormatHelper::ParseAVTrackInfo(const AVStream& avStream, Format &format)
{
    int64_t bitRate = static_cast<int64_t>(avStream.codecpar->bit_rate);
    if (bitRate > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_BITRATE, bitRate, format);
    } else {
        AVCODEC_LOGW("Parse bitrate info failed: %{public}" PRId64, bitRate);
    }

    if (avStream.codecpar->extradata_size > 0 && avStream.codecpar->extradata != nullptr) {
        PutBufferToFormat(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, avStream.codecpar->extradata,
                          avStream.codecpar->extradata_size, format);
    } else {
        AVCODEC_LOGW("Parse codec config info failed");
    }
}

void FFmpegFormatHelper::ParseVideoTrackInfo(const AVStream& avStream, Format &format)
{
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_WIDTH, static_cast<int32_t>(avStream.codecpar->width), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_HEIGHT, static_cast<int32_t>(avStream.codecpar->height), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_VIDEO_DELAY,
        static_cast<int32_t>(avStream.codecpar->video_delay), format);

    double frameRate = 0;
    if (avStream.avg_frame_rate.den == 0 || avStream.avg_frame_rate.num == 0) {
        frameRate = static_cast<double>(av_q2d(avStream.r_frame_rate));
    } else {
        frameRate = static_cast<double>(av_q2d(avStream.avg_frame_rate));
    }
    if (frameRate > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate, format);
    } else {
        AVCODEC_LOGW("Parse frame rate info failed: %{public}f", frameRate);
    }

    ParseInfoFromMetadata(avStream.metadata, MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, format);

    if (avStream.codecpar->codec_id == AV_CODEC_ID_HEVC) {
        ParseHvccBoxInfo(avStream, format);
        ParseColorBoxInfo(avStream, format);
    }
}

void FFmpegFormatHelper::ParseHvccBoxInfo(const AVStream& avStream, Format &format)
{
    int32_t profile = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHHEVCProfile(avStream.codecpar->profile));
    if (profile >= 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_PROFILE, profile, format);
    } else {
        AVCODEC_LOGW("Parse hevc profile info failed: %{public}d", profile);
    }
    int32_t level = static_cast<int32_t>(FFMpegConverter::ConvertFFMpegToOHHEVCLevel(avStream.codecpar->level));
    if (level >= 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_LEVEL, level, format);
    } else {
        AVCODEC_LOGW("Parse hevc level info failed: %{public}d", level);
    }
}

void FFmpegFormatHelper::ParseColorBoxInfo(const AVStream& avStream, Format &format)
{
    int colorRange = FFMpegConverter::ConvertFFMpegToOHColorRange(avStream.codecpar->color_range);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_RANGE_FLAG, static_cast<int32_t>(colorRange), format);

    ColorPrimary colorPrimaries = FFMpegConverter::ConvertFFMpegToOHColorPrimaries(avStream.codecpar->color_primaries);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, static_cast<int32_t>(colorPrimaries), format);

    TransferCharacteristic colorTrans = FFMpegConverter::ConvertFFMpegToOHColorTrans(avStream.codecpar->color_trc);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, static_cast<int32_t>(colorTrans), format);

    MatrixCoefficient colorMatrix = FFMpegConverter::ConvertFFMpegToOHColorMatrix(avStream.codecpar->color_space);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, static_cast<int32_t>(colorMatrix), format);

    ChromaLocation chromaLoc = FFMpegConverter::ConvertFFMpegToOHChromaLocation(avStream.codecpar->chroma_location);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_CHROMA_LOCATION, static_cast<int32_t>(chromaLoc), format);
}

void FFmpegFormatHelper::ParseImageTrackInfo(const AVStream& avStream, Format &format)
{
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_WIDTH, static_cast<int32_t>(avStream.codecpar->width), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_HEIGHT, static_cast<int32_t>(avStream.codecpar->height), format);
    AVPacket pkt = avStream.attached_pic;
    if (pkt.size > 0) {
        PutBufferToFormat(MediaDescriptionKey::MD_KEY_COVER, pkt.data, pkt.size, format);
    } else {
        AVCODEC_LOGW("Parse cover info failed: %{public}d", pkt.size);
    }
}

void FFmpegFormatHelper::ParseAudioTrackInfo(const AVStream& avStream, Format &format)
{
    int32_t sampelRate = static_cast<int32_t>(avStream.codecpar->sample_rate);
    int32_t channels = static_cast<int32_t>(avStream.codecpar->channels);
    int32_t frameSize = static_cast<int32_t>(avStream.codecpar->frame_size);
    if (sampelRate > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampelRate, format);
    } else {
        AVCODEC_LOGW("Parse sample rate info failed: %{public}d", sampelRate);
    }
    if (channels > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels, format);
    } else {
        AVCODEC_LOGW("Parse channel count info failed: %{public}d", channels);
    }
    if (frameSize > 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME, frameSize, format);
    } else {
        AVCODEC_LOGW("Parse frame rate info failed: %{public}d", frameSize);
    }
    int64_t channelLayout = static_cast<int64_t>(FFMpegConverter::ConvertFFToOHAudioChannelLayoutV2(
        avStream.codecpar->channel_layout, channels));
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, channelLayout, format);
    
    AudioSampleFormat fmt;
    if (!IsPCMStream(avStream.codecpar->codec_id)) {
        fmt = FFMpegConverter::ConvertFFMpegToOHAudioFormat(static_cast<AVSampleFormat>(avStream.codecpar->format));
    } else {
        fmt = FFMpegConverter::ConvertFFMpegAVCodecIdToOHAudioFormat(avStream.codecpar->codec_id);
    }
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, static_cast<int32_t>(fmt), format);

    if (avStream.codecpar->codec_id == AV_CODEC_ID_AAC) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1, format);
    } else if (avStream.codecpar->codec_id == AV_CODEC_ID_AAC_LATM) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 0, format);
    }
}

void FFmpegFormatHelper::ParseInfoFromMetadata(const AVDictionary* metadata, const std::string_view key, Format &format)
{
    AVDictionaryEntry *valPtr = nullptr;
    if (g_formatToString.count(key) != 0) {
        valPtr = av_dict_get(metadata, g_formatToString[key].c_str(), nullptr, AV_DICT_MATCH_CASE);
    } else {
        valPtr = av_dict_get(metadata, key.data(), nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        valPtr = av_dict_get(metadata, SwitchCase(std::string(key)).c_str(), nullptr, AV_DICT_MATCH_CASE);
    }
    if (valPtr == nullptr) {
        AVCODEC_LOGW("Parse %{public}s info failed", key.data());
    } else {
        if (key == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE) {
            PutInfoToFormat(key, static_cast<int32_t>(std::stoi(valPtr->value)), format);
        } else {
            PutInfoToFormat(key, valPtr->value, format);
        }
    }
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, int32_t value, Format& format)
{
    bool ret = format.PutIntValue(key, value);
    CHECK_AND_RETURN_LOG(ret, "Put %{public}s info failed", key.data());
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, int64_t value, Format& format)
{
    bool ret = format.PutLongValue(key, value);
    CHECK_AND_RETURN_LOG(ret, "Put %{public}s info failed", key.data());
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, float value, Format& format)
{
    bool ret = format.PutFloatValue(key, value);
    CHECK_AND_RETURN_LOG(ret, "Put %{public}s info failed", key.data());
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, double value, Format& format)
{
    bool ret = format.PutDoubleValue(key, value);
    CHECK_AND_RETURN_LOG(ret, "Put %{public}s info failed", key.data());
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, const std::string_view &value, Format& format)
{
    bool ret = format.PutStringValue(key, value);
    CHECK_AND_RETURN_LOG(ret, "Put %{public}s info failed", key.data());
}

void FFmpegFormatHelper::PutBufferToFormat(const std::string_view &key, const uint8_t *addr,
                                           size_t size, Format &format)
{
    bool ret = format.PutBuffer(key, addr, size);
    CHECK_AND_RETURN_LOG(ret, "Put %{public}s info failed", key.data());
}
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS