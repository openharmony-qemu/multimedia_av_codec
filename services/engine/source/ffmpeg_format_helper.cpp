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
#include "avcodec_dfx.h"
#include "avcodec_log.h"
#include "avcodec_info.h"
#include "ffmpeg_converter.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avutil.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FFmpegFormatHelper"};

    using PutFunction = bool (*)(const std::string_view &key, int32_t value);

    static std::map<AVMediaType, MediaType> g_convertFfmpegTrackType = {
        {AVMEDIA_TYPE_VIDEO, MediaType::MEDIA_TYPE_VID},
        {AVMEDIA_TYPE_AUDIO, MediaType::MEDIA_TYPE_AUD},
        {AVMEDIA_TYPE_SUBTITLE, MediaType::MEDIA_TYPE_SUBTITLE},
    };

    static std::map<AVCodecID, std::string_view> g_codecIdToMime = {
        {AV_CODEC_ID_MP3, CodecMimeType::AUDIO_MPEG},
        {AV_CODEC_ID_FLAC, CodecMimeType::AUDIO_FLAC},
        {AV_CODEC_ID_PCM_S16LE, CodecMimeType::AUDIO_RAW},
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

    static std::map<const char*, FileType> g_convertFfmpegFileType = {
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

    // static std::map<std::string_view, DataType> g_infoTypeMap = {
    //     // source format
    //     {MediaDescriptionKey::MD_KEY_TRACK_COUNT, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_DURATION, DataType::DATA_TYPE_INT64},
    //     {AVSourceFormat::SOURCE_HAS_VIDEO, DataType::DATA_TYPE_INT32},
    //     {AVSourceFormat::SOURCE_HAS_AUDIO, DataType::DATA_TYPE_INT32},
    //     {AVSourceFormat::SOURCE_FILE_TYPE, DataType::DATA_TYPE_INT32},
    //     {AVSourceFormat::SOURCE_TITLE, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_ARTIST, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_ALBUM, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_ALBUM_ARTIST, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_DATE, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_COMMENT, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_GENRE, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_COPYRIGHT, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_LANGUAGE, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_DESCRIPTION, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_LYRICS, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_AUTHOR, DataType::DATA_TYPE_STRING},
    //     {AVSourceFormat::SOURCE_COMPOSER, DataType::DATA_TYPE_STRING},
    //     // common track format
    //     {MediaDescriptionKey::MD_KEY_TRACK_TYPE, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_BITRATE, DataType::DATA_TYPE_INT64},
    //     {MediaDescriptionKey::MD_KEY_CODEC_MIME, DataType::DATA_TYPE_STRING},
    //     {MediaDescriptionKey::MD_KEY_COMPRESSION_LEVEL, DataType::DATA_TYPE_INT32},
    //     // video track format
    //     {MediaDescriptionKey::MD_KEY_WIDTH, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_HEIGHT, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_FRAME_RATE, DataType::DATA_TYPE_DOUBLE},
    //     {MediaDescriptionKey::MD_KEY_VIDEO_DELAY, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, DataType::DATA_TYPE_INT32},
    //     // audio track format
    //     {MediaDescriptionKey::MD_KEY_SAMPLE_RATE, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME, DataType::DATA_TYPE_INT32},
    //     {MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, DataType::DATA_TYPE_INT64},
    // };
    
    MediaType ConvertFFmpegMediaTypeToOHMediaType(AVMediaType mediaType)
    {
        auto ite = std::find_if(g_convertFfmpegTrackType.begin(), g_convertFfmpegTrackType.end(),
                                [&mediaType](const auto &item) -> bool { return item.first == mediaType; });
        if (ite == g_convertFfmpegTrackType.end()) {
            return MediaType::MEDIA_TYPE_UNKNOWN;
        }
        return ite->second;
    }

    bool StartWith(const char* name, const char* chars)
    {
        return !strncmp(name, chars, strlen(chars));
    }

    FileType GetFileTypeByName(const char *fileName, const bool hasVideo)
    {
        FileType fileType = FileType::FILE_TYPE_UNKNOW;
        if (StartWith(fileName, "mov,mp4,m4a")) {
            if (hasVideo) {
                fileType = FileType::FILE_TYPE_MP4;
            } else {
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
    bool hasCover = false;
    for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
        if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!hasCover && (avFormatContext.streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                hasCover = true;
                AVPacket pkt = avFormatContext.streams[i]->attached_pic;
                PutBufferToFormat(AVSourceFormat::SOURCE_COVER, pkt.data, pkt.size, format);
            } else {    
                hasVideo = true;
            }
        } else if (avFormatContext.streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            hasAudio = true;
        }
    }
    PutInfoToFormat(AVSourceFormat::SOURCE_HAS_VIDEO, static_cast<int32_t>(hasVideo), format);
    PutInfoToFormat(AVSourceFormat::SOURCE_HAS_AUDIO, static_cast<int32_t>(hasAudio), format);
    if (!hasCover) {
        AVCODEC_LOGW("Parse cover info failed");
    }

    PutInfoToFormat(AVSourceFormat::SOURCE_FILE_TYPE, static_cast<int32_t>(GetFileTypeByName(avFormatContext.iformat->name, hasVideo)), format);
    
    int64_t duration = avFormatContext.duration;
    if (duration == AV_NOPTS_VALUE) {
        for (uint32_t i = 0; i < avFormatContext.nb_streams; ++i) {
            auto streamDuration = avFormatContext.streams[i]->duration;
            if (streamDuration > duration) {
                duration = streamDuration;
            }
        }
    }
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_DURATION, static_cast<int64_t>(duration), format);

    for (std::string_view key: g_supportSourceFormat) {
        ParseInfoFromMetadata(avFormatContext.metadata, key, format);
    }
}

void FFmpegFormatHelper::ParseTrackInfo(const AVStream& avStream, Format &format)
{
    ParseCommonTrackInfo(avStream, format);
    if (avStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        ParseVideoTrackInfo(avStream, format);
    } else if (avStream.codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        ParseAudioTrackInfo(avStream, format);
    }
}

void FFmpegFormatHelper::ParseCommonTrackInfo(const AVStream& avStream, Format &format)
{
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_BITRATE, static_cast<int64_t>(avStream.codecpar->bit_rate), format);

    if (g_codecIdToMime.count(avStream.codecpar->codec_id) != 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_CODEC_MIME, g_codecIdToMime[avStream.codecpar->codec_id], format);
    } else {
        AVCODEC_LOGW("Parse mimeType info failed");
    }

    MediaType media_type = MediaType::MEDIA_TYPE_UNKNOWN;
    if (avStream.disposition & AV_DISPOSITION_ATTACHED_PIC) {
        media_type = MediaType::MEDIA_TYPE_COVER;
    } else {
        media_type = ConvertFFmpegMediaTypeToOHMediaType(avStream.codecpar->codec_type);
    }
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_TRACK_TYPE, static_cast<int32_t>(media_type), format);
    
    if (avStream.codecpar->extradata_size > 0 && avStream.codecpar->extradata != nullptr) {
        PutBufferToFormat(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, avStream.codecpar->extradata,
                          avStream.codecpar->extradata_size, format);
    } else {
        AVCODEC_LOGW("Parse codec config info failed");
    }

    auto codecContext = InitCodecContext(avStream);
    if (codecContext != nullptr) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_COMPRESSION_LEVEL, static_cast<int32_t>(codecContext->compression_level), format);
    } else {
        AVCODEC_LOGW("Parse compression level info failed");
    }
}

void FFmpegFormatHelper::ParseVideoTrackInfo(const AVStream& avStream, Format &format)
{
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_WIDTH, static_cast<int32_t>(avStream.codecpar->width), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_HEIGHT, static_cast<int32_t>(avStream.codecpar->height), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, static_cast<int32_t>(avStream.codecpar->video_delay), format);

    if (avStream.avg_frame_rate.den == 0 || avStream.avg_frame_rate.num == 0) {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_FRAME_RATE, static_cast<double>(av_q2d(avStream.r_frame_rate)), format);
    } else {
        PutInfoToFormat(MediaDescriptionKey::MD_KEY_FRAME_RATE, static_cast<double>(av_q2d(avStream.avg_frame_rate)), format);
    }

    ParseInfoFromMetadata(avStream.metadata, MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, format);
}

void FFmpegFormatHelper::ParseAudioTrackInfo(const AVStream& avStream, Format &format)
{
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, static_cast<int32_t>(avStream.codecpar->sample_rate), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, static_cast<int32_t>(avStream.codecpar->channels), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME, static_cast<int32_t>(avStream.codecpar->frame_size), format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT,
        static_cast<int64_t>(FFMpegConverter::ConvertFFToOHAudioChannelLayout(avStream.codecpar->channel_layout)), format);
    
    auto sampleFormat = static_cast<AVSampleFormat>(avStream.codecpar->format);
    PutInfoToFormat(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                    FFMpegConverter::ConvertFFMpegToOHAudioFormat(sampleFormat), format);

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
        AVCODEC_LOGW("Parse %{public}s info failed", key.data());
    } else {
        if (key == MediaDescriptionKey::MD_KEY_ROTATION_ANGLE) {
            PutInfoToFormat(key, static_cast<int32_t>(std::stoi(valPtr->value)), format);    
        } else {
            PutInfoToFormat(key, valPtr->value, format);
        }
    }
}

// template <typename T>
// void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, const T value, Format &format)
// {
//     bool ret = false;
//     if (g_infoTypeMap.count(key) == 0) {
//         AVCODEC_LOGD("Put %{public}s info failed, because type is unknown", key.data());
//         return;
//     }
//     switch (g_infoTypeMap[key]) {
//         case DATA_TYPE_INT32:
//             ret = format.PutIntValue(key ,value);
//             break;
//         case DATA_TYPE_INT64:
//             ret = format.PutLongValue(key ,value);
//             break;
//         case DATA_TYPE_FLOAT:
//             ret = format.PutFloatValue(key ,value);
//             break;
//         case DATA_TYPE_DOUBLE:
//             ret = format.PutDoubleValue(key ,value);
//             break;
//         case DATA_TYPE_STRING:
//             ret = format.PutStringValue(key ,value);
//             break;
//         default:
//             break;
//     };
//     if (!ret) {
//         AVCODEC_LOGW("Put %{public}s info failed", key.data());
//     }
// }

std::shared_ptr<AVCodecContext> FFmpegFormatHelper::InitCodecContext(const AVStream& avStream)
{
    auto codecContext = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(nullptr), [](AVCodecContext* p) {
        if (p) {
            avcodec_free_context(&p);
        }
    });
    if (codecContext == nullptr) {
        AVCODEC_LOGE("cannot create ffmpeg codecContext");
        return nullptr;
    }
    int ret = avcodec_parameters_to_context(codecContext.get(), avStream.codecpar);
    if (ret < 0) {
        AVCODEC_LOGE("avcodec_parameters_to_context failed with error: %{public}s", FFMpegConverter::AVStrError(ret).c_str());
        return nullptr;
    }
    codecContext->workaround_bugs = static_cast<uint32_t>(codecContext->workaround_bugs) | FF_BUG_AUTODETECT;
    codecContext->err_recognition = 1;
    return codecContext;
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, int32_t value, Format& format)
{
    bool ret = format.PutIntValue(key ,value);
    if (!ret) {
        AVCODEC_LOGW("Put %{public}s info failed", key.data());
    }
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, int64_t value, Format& format)
{
    bool ret = format.PutLongValue(key ,value);
    if (!ret) {
        AVCODEC_LOGW("Put %{public}s info failed", key.data());
    }
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, float value, Format& format)
{
    bool ret = format.PutFloatValue(key ,value);
    if (!ret) {
        AVCODEC_LOGW("Put %{public}s info failed", key.data());
    }
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, double value, Format& format)
{
    bool ret = format.PutDoubleValue(key ,value);
    if (!ret) {
        AVCODEC_LOGW("Put %{public}s info failed", key.data());
    }
}

void FFmpegFormatHelper::PutInfoToFormat(const std::string_view &key, const std::string_view &value, Format& format)
{
    bool ret = format.PutStringValue(key ,value);
    if (!ret) {
        AVCODEC_LOGW("Put %{public}s info failed", key.data());
    }
}

void FFmpegFormatHelper::PutBufferToFormat(const std::string_view &key, const uint8_t *dataAddr, size_t size, Format &format)
{
    bool ret = format.PutBuffer(key, dataAddr, size);
    if (!ret) {
        AVCODEC_LOGW("Put %{public}s info failed", key.data());
    }
}

} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS