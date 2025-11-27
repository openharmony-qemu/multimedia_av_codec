/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "DemuxerLogCompressor"

#include <unordered_map>
#include <sstream>
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "common/log.h"
#include "meta/format.h"
#include "demuxer_log_compressor.h"
#include "ffmpeg_format_helper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "DemuxerLogCompressor" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
static std::unordered_map<TagType, std::string> g_formatToIndex = {
    {Tag::AUDIO_AAC_IS_ADTS,              "adts"},
    {Tag::AUDIO_BITS_PER_CODED_SAMPLE,    "bitsPerCodedSmp"},
    {Tag::AUDIO_BITS_PER_RAW_SAMPLE,      "bitsPerRawSmp"},
    {Tag::AUDIO_CHANNEL_COUNT,            "channelCnt"},
    {Tag::AUDIO_CHANNEL_LAYOUT,           "channelLayout"},
    {Tag::AUDIO_OUTPUT_CHANNELS,          "outChannelCnt"},
    {Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT,    "outChannelLayout"},
    {Tag::AUDIO_SAMPLE_FORMAT,            "smpFmt"},
    {Tag::AUDIO_SAMPLE_PER_FRAME,         "smpFrm"},
    {Tag::AUDIO_SAMPLE_RATE,              "smpRate"},
    {Tag::AUDIO_OBJECT_NUMBER,            "objCnt"},
    {Tag::AUDIO_HOA_ORDER,                "haoOrder"},
    {Tag::AUDIO_SOUNDBED_CHANNELS_NUMBER, "soundbedChannelsCnt"},
    {Tag::MEDIA_ALBUM,                    "album"},
    {Tag::MEDIA_ALBUM_ARTIST,             "albumArt"},
    {Tag::MEDIA_ARTIST,                   "art"},
    {Tag::MEDIA_AUTHOR,                   "auth"},
    {Tag::MEDIA_BITRATE,                  "bitRate"},
    {Tag::MEDIA_CODEC_CONFIG,             "csd"},
    {Tag::MEDIA_COMMENT,                  "comment"},
    {Tag::MEDIA_COMPOSER,                 "composer"},
    {Tag::MEDIA_CONTAINER_START_TIME,     "containStartTime"},
    {Tag::MEDIA_COPYRIGHT,                "copyright"},
    {Tag::MEDIA_COVER,                    "cover"},
    {Tag::MEDIA_CREATION_TIME,            "createTime"},
    {Tag::MEDIA_DATE,                     "date"},
    {Tag::MEDIA_DESCRIPTION,              "desc"},
    {Tag::MEDIA_DURATION,                 "duration"},
    {Tag::MEDIA_FILE_TYPE,                "fileType"},
    {Tag::MEDIA_GENRE,                    "gnre"},
    {Tag::MEDIA_HAS_AUDIO,                "hasAud"},
    {Tag::MEDIA_HAS_SUBTITLE,             "hasSub"},
    {Tag::MEDIA_HAS_TIMEDMETA,            "hasTimed"},
    {Tag::MEDIA_HAS_AUXILIARY,            "hasAuxl"},
    {Tag::MEDIA_HAS_VIDEO,                "hasVid"},
    {Tag::MEDIA_LANGUAGE,                 "lang"},
    {Tag::MEDIA_LATITUDE,                 "latitude"},
    {Tag::MEDIA_LEVEL,                    "level"},
    {Tag::MEDIA_LONGITUDE,                "longitude"},
    {Tag::MEDIA_LYRICS,                   "lyrics"},
    {Tag::MEDIA_PROFILE,                  "profile"},
    {Tag::MEDIA_START_TIME,               "trackStartTime"},
    {Tag::MEDIA_TITLE,                    "title"},
    {Tag::MEDIA_TRACK_COUNT,              "trackCnt"},
    {Tag::MEDIA_TYPE,                     "trackType"},
    {Tag::MIME_TYPE,                      "mime"},
    {Tag::TIMED_METADATA_KEY,             "metaKey"},
    {Tag::TIMED_METADATA_SRC_TRACK,       "metaSrcTrack"},
    {Tag::VIDEO_CHROMA_LOCATION,          "chromaLoc"},
    {Tag::VIDEO_COLOR_MATRIX_COEFF,       "colorMatrix"},
    {Tag::VIDEO_COLOR_PRIMARIES,          "colorPri"},
    {Tag::VIDEO_COLOR_RANGE,              "colorRange"},
    {Tag::VIDEO_COLOR_TRC,                "colorTrc"},
    {Tag::VIDEO_DELAY,                    "delay"},
    {Tag::VIDEO_FRAME_RATE,               "frmRate"},
    {Tag::VIDEO_H265_LEVEL,               "265level"},
    {Tag::VIDEO_H265_PROFILE,             "265profile"},
    {Tag::VIDEO_HEIGHT,                   "h"},
    {Tag::VIDEO_IS_HDR_VIVID,             "hdrVivid"},
    {Tag::VIDEO_ORIENTATION_TYPE,         "orient"},
    {Tag::VIDEO_ROTATION,                 "rotate"},
    {Tag::VIDEO_SAR,                      "sar"},
    {Tag::VIDEO_WIDTH,                    "w"},
    {Tag::AUDIO_MAX_INPUT_SIZE,           "maxFrame"},
    {Tag::TRACK_REFERENCE_TYPE,           "refType"},
    {Tag::TRACK_DESCRIPTION,              "trackDesc"},
    {Tag::REFERENCE_TRACK_IDS,            "refIds"},
    {Tag::IS_GLTF,                        "isGltf"},
    {Tag::GLTF_OFFSET,                    "gltfOffset"},
    {Tag::AUDIO_BLOCK_ALIGN,              "blockAlign"},
    {Tag::ORIGINAL_CODEC_NAME,            "codecName"},
};

std::string DemuxerLogCompressor::FormatTagSerialize(Format& format)
{
    std::stringstream dumpStr;
    auto meta = format.GetMeta();
    FALSE_RETURN_V_MSG_E(meta != nullptr, "", "Meta is nullptr");
    for (auto iter = meta->begin(); iter != meta->end(); ++iter) {
        if (g_formatToIndex.find(iter->first) == g_formatToIndex.end()) {
            dumpStr << iter->first << "=" << AnyCast<std::string>(iter->second) << "|";
            continue;
        }
        switch (format.GetValueType(iter->first)) {
            case FORMAT_TYPE_INT32:
                dumpStr << g_formatToIndex[iter->first] << "=" << std::to_string(AnyCast<int32_t>(iter->second)) << "|";
                break;
            case FORMAT_TYPE_INT64:
                dumpStr << g_formatToIndex[iter->first] << "=" << std::to_string(AnyCast<int64_t>(iter->second)) << "|";
                break;
            case FORMAT_TYPE_FLOAT:
                dumpStr << g_formatToIndex[iter->first] << "=" << std::to_string(AnyCast<float>(iter->second)) << "|";
                break;
            case FORMAT_TYPE_DOUBLE:
                dumpStr << g_formatToIndex[iter->first] << "=" << std::to_string(AnyCast<double>(iter->second)) << "|";
                break;
            case FORMAT_TYPE_STRING:
                dumpStr << g_formatToIndex[iter->first] << "=" << AnyCast<std::string>(iter->second) << "|";
                break;
            case FORMAT_TYPE_ADDR: {
                Any *value = const_cast<Any *>(&(iter->second));
                if (AnyCast<std::vector<uint8_t>>(value) != nullptr) {
                    dumpStr << g_formatToIndex[iter->first] << ", size="
                            << (AnyCast<std::vector<uint8_t>>(value))->size() << "|";
                }
                break;
            }
            default:
                MEDIA_LOG_E("Stringify failed, Key " PUBLIC_LOG_S, iter->first.c_str());
        }
    }
    return dumpStr.str();
}

void DemuxerLogCompressor::StringifyMeta(Meta meta, int32_t trackIndex)
{
    OHOS::Media::Format format;
    for (TagType key: g_supportSourceFormat) {
        if (meta.Find(std::string(key)) != meta.end()) {
            meta.SetData(std::string(key), "*");
        }
    }
    if (meta.Find(std::string(Tag::MEDIA_CONTAINER_START_TIME)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_CONTAINER_START_TIME), "*");
    }
    if (meta.Find(std::string(Tag::MEDIA_START_TIME)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_START_TIME), "*");
    }
    if (meta.Find(std::string(Tag::MEDIA_LATITUDE)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_LATITUDE), "*");
    }
    if (meta.Find(std::string(Tag::MEDIA_LONGITUDE)) != meta.end()) {
        meta.SetData(std::string(Tag::MEDIA_LONGITUDE), "*");
    }
    if (meta.Find(std::string(Tag::TIMED_METADATA_KEY)) != meta.end()) {
        meta.SetData(std::string(Tag::TIMED_METADATA_KEY), "*");
    }
    if (meta.Find(std::string(Tag::TRACK_DESCRIPTION)) != meta.end()) {
        meta.SetData(std::string(Tag::TRACK_DESCRIPTION), "*");
    }
    if (meta.Find(std::string(Tag::REFERENCE_TRACK_IDS)) != meta.end()) {
        meta.SetData(std::string(Tag::REFERENCE_TRACK_IDS), "*");
    }
    if (meta.Find(std::string(Tag::TIMED_METADATA_SRC_TRACK)) != meta.end()) {
        meta.SetData(std::string(Tag::TIMED_METADATA_SRC_TRACK), "*");
    }
    format.SetMeta(std::make_shared<Meta>(meta));
    if (trackIndex < 0) {
        MEDIA_LOG_I("[source]: " PUBLIC_LOG_S, FormatTagSerialize(format).c_str());
    } else {
        MEDIA_LOG_I("[track " PUBLIC_LOG_D32 "]: " PUBLIC_LOG_S, trackIndex, FormatTagSerialize(format).c_str());
    }
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS