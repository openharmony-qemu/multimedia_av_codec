/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mpeg4_demuxer_plugin_unit_test.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "file_server_demo.h"
#include "mpeg4_sample_memory.h"
#include "rbsp_context.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
Status ConvertTimeScaleRound(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result);
Status ConvertTimeScaleRoundToTimeBase(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result);
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS

namespace {
const std::string TEST_MP4_PATH_H264_B = "/data/test/media/test_264_B_Gop25_4sec_cover.mp4";
const std::string TEST_MP4_PATH_H265_B = "/data/test/media/test_265_B_Gop25_4sec.mp4";
const std::string TEST_MP4_PATH_HEVC_AUX = "/data/test/media/muxer_auxl_265.mp4";
const std::string TEST_MP4_PATH_HEVC_AVC_AAC = "/data/test/media/muxer_auxl_265_264_aac.mp4";
const std::string TEST_MP4_PATH_WEBVTT = "/data/test/media/webvtt.mp4";
const std::string TEST_MP4_MULTIPLE_VIDEO_TRACKS = "/data/test/media/h264_double_video_audio.mp4";
const std::string TEST_URI_PATH = "http://127.0.0.1:46666/";
const std::string TEST_RELATIVE_PATH = "/data/test/media/";
const std::string TEST_DEMUXER_RESOURCE_DIR = "/data/test/media";
const std::string FFMPEG_MP4_PLUGIN_NAME = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
const std::string WEBVTT_MIME = "text/vtt";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
constexpr int32_t WAIT_RETRY_TIMES = 20;
constexpr uint32_t INVALID_TRACK_ID = static_cast<uint32_t>(-1);
constexpr uint32_t WEAK_NETWORK_SHORT_TIMEOUT_MS = 10;
constexpr uint32_t WEAK_NETWORK_FAIL_DELAY_MS = 20;
constexpr int64_t WEAK_NETWORK_FIRST_READ_FAIL_OFFSET = 0;
constexpr size_t WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT = 1;
constexpr size_t WEAK_NETWORK_THREAD_RETRY_FAIL_COUNT = 20;
constexpr size_t MAX_WEBVTT_SAMPLE_READ_COUNT = 100000;
constexpr int64_t GLTF_OFFSET_3DGS = 3526448;
constexpr int64_t TEST_MS_TO_US = 1000;
constexpr int64_t TEST_US_TO_NS = 1000;
std::unique_ptr<FileServerDemo> g_server = nullptr;

std::vector<SeekExpectedResult> GetExplicitSeekExpectedResults()
{
    return {
        {0, SeekMode::SEEK_NEXT_SYNC, true, {103, 174}},
        {0, SeekMode::SEEK_PREVIOUS_SYNC, true, {103, 174}},
        {0, SeekMode::SEEK_CLOSEST_SYNC, true, {103, 174}},
        {2000, SeekMode::SEEK_NEXT_SYNC, true, {53, 90}},
        {2000, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2000, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {1920, SeekMode::SEEK_NEXT_SYNC, true, {53, 90}},
        {1920, SeekMode::SEEK_PREVIOUS_SYNC, true, {78, 134}},
        {1920, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 90}},
        {2160, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2160, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2160, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2200, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2200, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2200, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2440, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2440, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2440, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2600, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2600, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2600, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {2700, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2700, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2700, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {4080, SeekMode::SEEK_NEXT_SYNC, false, {}},
        {4080, SeekMode::SEEK_PREVIOUS_SYNC, true, {3, 5}},
        {4080, SeekMode::SEEK_CLOSEST_SYNC, true, {3, 5}},
        {4100, SeekMode::SEEK_NEXT_SYNC, false, {}},
        {4100, SeekMode::SEEK_PREVIOUS_SYNC, true, {3, 5}},
        {4100, SeekMode::SEEK_CLOSEST_SYNC, true, {3, 5}},
    };
}

struct MetaDumpItem {
    TagType tag;
    std::string name;
};

struct SetDataSourceRunResult {
    Status ret = Status::ERROR_UNKNOWN;
    Status mediaInfoRet = Status::ERROR_UNKNOWN;
    bool hasMediaInfo = false;
    MediaInfo mediaInfo;
};

struct FixedSeekSampleExpected {
    int64_t seekTimeMs = 0;
    SeekMode seekMode = SeekMode::SEEK_PREVIOUS_SYNC;
    int64_t realSeekTimeNs = 0;
    int64_t ptsUs = 0;
    int64_t dtsUs = 0;
    int64_t durationUs = 0;
};

struct KnownSetDataSourceDiffResource {
    std::string filePath;
    std::vector<std::string> issueFields;
};

std::vector<FixedSeekSampleExpected> GetH264BFfprobeSeekExpectedResults()
{
    return {
        {2440, SeekMode::SEEK_PREVIOUS_SYNC, 2440000000LL, 2000000, 1920000, 40000},
        {2440, SeekMode::SEEK_NEXT_SYNC, 2440000000LL, 3000000, 2920000, 40000},
        {2440, SeekMode::SEEK_CLOSEST_SYNC, 2440000000LL, 2000000, 1920000, 40000},
        {2600, SeekMode::SEEK_CLOSEST_SYNC, 2600000000LL, 3000000, 2920000, 40000},
    };
}

struct MetaCompareDiff {
    std::string scope;
    std::string field;
    std::string ffmpegValue;
    std::string mpeg4Value;
};

const std::vector<MetaDumpItem>& GetSourceDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::MEDIA_TITLE, "title"},
        {Tag::MEDIA_ARTIST, "artist"},
        {Tag::MEDIA_ALBUM, "album"},
        {Tag::MEDIA_ALBUM_ARTIST, "albumArt"},
        {Tag::MEDIA_DATE, "date"},
        {Tag::MEDIA_COMMENT, "comment"},
        {Tag::MEDIA_GENRE, "gnre"},
        {Tag::MEDIA_COPYRIGHT, "copyright"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::MEDIA_DESCRIPTION, "desc"},
        {Tag::MEDIA_LYRICS, "lyrics"},
        {Tag::MEDIA_AUTHOR, "auth"},
        {Tag::MEDIA_COMPOSER, "composer"},
        {Tag::MEDIA_CREATION_TIME, "createTime"},
        {Tag::MEDIA_DURATION, "duration"},
        {Tag::MEDIA_FILE_TYPE, "fileType"},
        {Tag::MEDIA_HAS_AUDIO, "hasAud"},
        {Tag::MEDIA_HAS_AUXILIARY, "hasAuxl"},
        {Tag::MEDIA_HAS_SUBTITLE, "hasSub"},
        {Tag::MEDIA_HAS_TIMEDMETA, "hasTimed"},
        {Tag::MEDIA_HAS_VIDEO, "hasVid"},
        {Tag::MEDIA_CONTAINER_START_TIME, "containStartTime"},
        {Tag::MEDIA_TIME_SCALE, "timeScale"},
        {Tag::MEDIA_TRACK_COUNT, "trackCnt"},
        {Tag::MEDIA_ENCODER, "encoder"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetVideoTrackDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::MEDIA_BITRATE, "bitRate"},
        {Tag::MEDIA_CODEC_CONFIG, "csd"},
        {Tag::MIME_TYPE, "mime"},
        {Tag::VIDEO_FRAME_RATE, "frmRate"},
        {Tag::VIDEO_HEIGHT, "h"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::ORIGINAL_CODEC_NAME, "codecName"},
        {Tag::VIDEO_ROTATION, "rotate"},
        {Tag::MEDIA_TIME_SCALE, "timeScale"},
        {Tag::MEDIA_START_TIME, "trackStartTime"},
        {Tag::MEDIA_TYPE, "trackType"},
        {Tag::VIDEO_DELAY, "delay"},
        {Tag::VIDEO_ORIENTATION_TYPE, "orient"},
        {Tag::VIDEO_SAR, "sar"},
        {Tag::VIDEO_WIDTH, "w"},
        {Tag::MEDIA_COVER, "cover"},
        {Tag::TRACK_REFERENCE_TYPE, "refType"},
        {Tag::TRACK_DESCRIPTION, "trackDesc"},
        {Tag::REFERENCE_TRACK_IDS, "refIds"},
        {Tag::IS_GLTF, "isGltf"},
        {Tag::GLTF_OFFSET, "gltfOffset"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetAudioTrackDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::AUDIO_AAC_IS_ADTS, "adts"},
        {Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT, "outChannelLayout"},
        {Tag::AUDIO_OUTPUT_CHANNELS, "outChannelCnt"},
        {Tag::AUDIO_BLOCK_ALIGN, "blockAlign"},
        {Tag::AUDIO_SAMPLE_FORMAT, "smpFmt"},
        {Tag::AUDIO_SAMPLE_PER_FRAME, "smpFrm"},
        {Tag::MEDIA_BITRATE, "bitRate"},
        {Tag::AUDIO_BITS_PER_CODED_SAMPLE, "bitsPerCodedSmp"},
        {Tag::AUDIO_BITS_PER_RAW_SAMPLE, "bitsPerRawSmp"},
        {Tag::AUDIO_CHANNEL_COUNT, "channelCnt"},
        {Tag::AUDIO_CHANNEL_LAYOUT, "channelLayout"},
        {Tag::MEDIA_CODEC_CONFIG, "csd"},
        {Tag::MIME_TYPE, "mime"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::ORIGINAL_CODEC_NAME, "codecName"},
        {Tag::AUDIO_SAMPLE_RATE, "smpRate"},
        {Tag::MEDIA_TIME_SCALE, "timeScale"},
        {Tag::MEDIA_START_TIME, "trackStartTime"},
        {Tag::MEDIA_TYPE, "trackType"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetTextTrackDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::MEDIA_BITRATE, "bitRate"},
        {Tag::MIME_TYPE, "mime"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::ORIGINAL_CODEC_NAME, "codecName"},
        {Tag::MEDIA_TIME_SCALE, "timeScale"},
        {Tag::MEDIA_START_TIME, "trackStartTime"},
        {Tag::MEDIA_TYPE, "trackType"},
        {Tag::TIMED_METADATA_KEY, "metaKey"},
        {Tag::TIMED_METADATA_SRC_TRACK, "metaSrcTrack"},
        {Tag::TRACK_REFERENCE_TYPE, "refType"},
        {Tag::TRACK_DESCRIPTION, "trackDesc"},
        {Tag::REFERENCE_TRACK_IDS, "refIds"},
        {Tag::IS_GLTF, "isGltf"},
        {Tag::GLTF_OFFSET, "gltfOffset"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetTrackDumpItems(const Meta& meta)
{
    Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
    if (!meta.GetData(Tag::MEDIA_TYPE, mediaType)) {
        return GetVideoTrackDumpItems();
    }
    if (mediaType == Plugins::MediaType::AUDIO) {
        return GetAudioTrackDumpItems();
    }
    if (mediaType == Plugins::MediaType::SUBTITLE || mediaType == Plugins::MediaType::TIMEDMETA) {
        return GetTextTrackDumpItems();
    }
    return GetVideoTrackDumpItems();
}

template<typename EnumType>
bool AppendEnumValue(const Meta& meta, const TagType& tag, std::stringstream& dumpStr)
{
    EnumType value {};
    if (!meta.GetData(tag, value)) {
        return false;
    }
    dumpStr << static_cast<int64_t>(value);
    return true;
}

bool AppendKnownEnumValue(const Meta& meta, const TagType& tag, std::stringstream& dumpStr)
{
    if (tag == Tag::MEDIA_FILE_TYPE) {
        return AppendEnumValue<Plugins::FileType>(meta, tag, dumpStr);
    }
    if (tag == Tag::MEDIA_TYPE) {
        return AppendEnumValue<Plugins::MediaType>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_ROTATION) {
        return AppendEnumValue<Plugins::VideoRotation>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_ORIENTATION_TYPE) {
        return AppendEnumValue<Plugins::VideoOrientationType>(meta, tag, dumpStr);
    }
    if (tag == Tag::AUDIO_SAMPLE_FORMAT) {
        return AppendEnumValue<Plugins::AudioSampleFormat>(meta, tag, dumpStr);
    }
    if (tag == Tag::AUDIO_CHANNEL_LAYOUT || tag == Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT) {
        return AppendEnumValue<Plugins::AudioChannelLayout>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_COLOR_PRIMARIES) {
        return AppendEnumValue<Plugins::ColorPrimary>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_COLOR_TRC) {
        return AppendEnumValue<Plugins::TransferCharacteristic>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_COLOR_MATRIX_COEFF) {
        return AppendEnumValue<Plugins::MatrixCoefficient>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_HDR_TYPE || tag == Tag::VIDEO_HDR_COMPATIBILITY) {
        return AppendEnumValue<Plugins::HDRType>(meta, tag, dumpStr);
    }
    return false;
}

bool AppendPrimitiveMetaValue(const Meta& meta, const TagType& tag, std::stringstream& dumpStr)
{
    bool boolValue = false;
    if (meta.GetData(tag, boolValue)) {
        dumpStr << static_cast<int32_t>(boolValue);
        return true;
    }
    int32_t int32Value = 0;
    if (meta.GetData(tag, int32Value)) {
        dumpStr << int32Value;
        return true;
    }
    uint32_t uint32Value = 0;
    if (meta.GetData(tag, uint32Value)) {
        dumpStr << uint32Value;
        return true;
    }
    int64_t int64Value = 0;
    if (meta.GetData(tag, int64Value)) {
        dumpStr << int64Value;
        return true;
    }
    uint64_t uint64Value = 0;
    if (meta.GetData(tag, uint64Value)) {
        dumpStr << uint64Value;
        return true;
    }
    float floatValue = 0.0F;
    if (meta.GetData(tag, floatValue)) {
        dumpStr << std::to_string(floatValue);
        return true;
    }
    double doubleValue = 0.0;
    if (meta.GetData(tag, doubleValue)) {
        dumpStr << std::to_string(doubleValue);
        return true;
    }
    std::string stringValue;
    if (meta.GetData(tag, stringValue)) {
        dumpStr << stringValue;
        return true;
    }
    return AppendKnownEnumValue(meta, tag, dumpStr);
}

std::string GetMetaDumpValue(const Meta& meta, const MetaDumpItem& item)
{
    std::vector<uint8_t> bytes;
    if (meta.GetData(item.tag, bytes)) {
        return "size=" + std::to_string(bytes.size());
    }
    std::vector<uint32_t> uint32Values;
    if (meta.GetData(item.tag, uint32Values)) {
        return "size=" + std::to_string(uint32Values.size());
    }
    std::vector<int32_t> int32Values;
    if (meta.GetData(item.tag, int32Values)) {
        return "size=" + std::to_string(int32Values.size());
    }
    std::vector<int64_t> int64Values;
    if (meta.GetData(item.tag, int64Values)) {
        return "size=" + std::to_string(int64Values.size());
    }
    std::stringstream dumpStr;
    if (!AppendPrimitiveMetaValue(meta, item.tag, dumpStr)) {
        dumpStr << "<unsupported>";
    }
    return dumpStr.str();
}

std::vector<MetaDumpItem> MergeDumpItems(const std::vector<MetaDumpItem>& lhs, const std::vector<MetaDumpItem>& rhs)
{
    std::vector<MetaDumpItem> items = lhs;
    for (const auto& rhsItem : rhs) {
        auto iter = std::find_if(items.begin(), items.end(), [&rhsItem](const MetaDumpItem& item) {
            return item.tag == rhsItem.tag;
        });
        if (iter == items.end()) {
            items.emplace_back(rhsItem);
        }
    }
    return items;
}

std::string GetComparableMetaValue(const Meta& meta, const MetaDumpItem& item)
{
    if (meta.Find(item.tag) == meta.end()) {
        return "<missing>";
    }
    return GetMetaDumpValue(meta, item);
}

bool ContainsIssueField(const std::vector<std::string>& issueFields, const std::string& field)
{
    return std::find(issueFields.begin(), issueFields.end(), field) != issueFields.end();
}

void AppendMetaDiffs(std::vector<MetaCompareDiff>& diffs, const std::string& scope, const Meta& ffmpegMeta,
    const Meta& mpeg4Meta, const std::vector<MetaDumpItem>& items)
{
    for (const auto& item : items) {
        std::string ffmpegValue = GetComparableMetaValue(ffmpegMeta, item);
        std::string mpeg4Value = GetComparableMetaValue(mpeg4Meta, item);
        if (ffmpegValue == mpeg4Value) {
            continue;
        }
        diffs.emplace_back(MetaCompareDiff {scope, item.name, ffmpegValue, mpeg4Value});
    }
}

void PrintSetDataSourceDiff(const MetaCompareDiff& diff)
{
    std::cout << "  " << diff.scope << "." << diff.field << ": ffmpeg=" << diff.ffmpegValue;
    std::cout << ", mpeg4=" << diff.mpeg4Value << std::endl;
}

void PrintSetDataSourceDiffGroupHeader(const std::string& diffType, const std::string& filePath)
{
    std::cout << "[SetDataSourceCompatibility][" << diffType << "] file=" << filePath << std::endl;
}

void ExpectSetDataSourceDiffs(const std::string& filePath, const std::vector<MetaCompareDiff>& diffs,
    const std::vector<std::string>& ignoredFields)
{
    bool hasKnownDiff = false;
    bool hasUnexpectedDiff = false;
    for (const auto& diff : diffs) {
        if (ContainsIssueField(ignoredFields, diff.field)) {
            if (!hasKnownDiff) {
                PrintSetDataSourceDiffGroupHeader("known", filePath);
                hasKnownDiff = true;
            }
            PrintSetDataSourceDiff(diff);
            continue;
        }
        if (!hasUnexpectedDiff) {
            PrintSetDataSourceDiffGroupHeader("unexpected", filePath);
            hasUnexpectedDiff = true;
        }
        PrintSetDataSourceDiff(diff);
        EXPECT_EQ(diff.ffmpegValue, diff.mpeg4Value) << diff.scope << "." << diff.field;
    }
}

void ExpectSetDataSourceCompatible(const std::string& filePath, const SetDataSourceRunResult& ffmpegResult,
    const SetDataSourceRunResult& mpeg4Result, const std::vector<std::string>& ignoredFields)
{
    SCOPED_TRACE(filePath);
    std::vector<MetaCompareDiff> diffs;
    if (ffmpegResult.ret != mpeg4Result.ret) {
        diffs.emplace_back(MetaCompareDiff {"[SetDataSource]", "ret",
            std::to_string(static_cast<int32_t>(ffmpegResult.ret)),
            std::to_string(static_cast<int32_t>(mpeg4Result.ret))});
    }
    if (ffmpegResult.hasMediaInfo != mpeg4Result.hasMediaInfo) {
        diffs.emplace_back(MetaCompareDiff {"[GetMediaInfo]", "available",
            std::to_string(static_cast<int32_t>(ffmpegResult.hasMediaInfo)),
            std::to_string(static_cast<int32_t>(mpeg4Result.hasMediaInfo))});
    }
    ExpectSetDataSourceDiffs(filePath, diffs, ignoredFields);
    if (!ffmpegResult.hasMediaInfo || !mpeg4Result.hasMediaInfo) {
        return;
    }

    diffs.clear();
    AppendMetaDiffs(diffs, "[source]", ffmpegResult.mediaInfo.general, mpeg4Result.mediaInfo.general,
        GetSourceDumpItems());
    if (ffmpegResult.mediaInfo.tracks.size() != mpeg4Result.mediaInfo.tracks.size()) {
        diffs.emplace_back(MetaCompareDiff {"[track]", "track.count",
            std::to_string(ffmpegResult.mediaInfo.tracks.size()),
            std::to_string(mpeg4Result.mediaInfo.tracks.size())});
    }
    size_t trackCount = std::min(ffmpegResult.mediaInfo.tracks.size(), mpeg4Result.mediaInfo.tracks.size());
    for (size_t index = 0; index < trackCount; ++index) {
        const auto& ffmpegTrack = ffmpegResult.mediaInfo.tracks[index];
        const auto& mpeg4Track = mpeg4Result.mediaInfo.tracks[index];
        auto items = MergeDumpItems(GetTrackDumpItems(ffmpegTrack), GetTrackDumpItems(mpeg4Track));
        AppendMetaDiffs(diffs, "[track " + std::to_string(index) + "]", ffmpegTrack, mpeg4Track, items);
    }
    ExpectSetDataSourceDiffs(filePath, diffs, ignoredFields);
}

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool HasDemuxerResourceSuffix(const std::string& fileName)
{
    static const std::vector<std::string> demuxerSuffixes = {
        ".mov", ".mp4", ".m4a", ".3gp", ".3g2", ".mj2", ".psp", ".m4b", ".ism", ".ismv", ".isma",
        ".f4v", ".avif", ".heic", ".heif"
    };
    std::string lowerName = ToLowerAscii(fileName);
    return std::any_of(demuxerSuffixes.begin(), demuxerSuffixes.end(), [&lowerName](const std::string& suffix) {
        return lowerName.size() >= suffix.size() &&
            lowerName.compare(lowerName.size() - suffix.size(), suffix.size(), suffix) == 0;
    });
}

bool IsSupportedFile(const std::string& fileName)
{
    static const std::vector<std::string> supportFiles = {
        "2obj_16bit_44100Hz_64kbps.mp4",
        "2obj_44100Hz_16bit_32k.m4a",
        "2obj_44100Hz_16bit_32k.mp4",
        "5_1_4_4obj_16bit_48000Hz_1232kbps.mp4",
        "5_1_4_16bit_32000Hz_704kbps.mp4",
        "5_1_4_16bit_96000Hz_704kbps.mp4",
        "5_1_4_24bit_48000Hz_704kbps.mp4",
        "7_1_4_16bit_48000Hz_832kbps.mp4",
        "aac_hdrvivid.mp4",
        "aigc_str.m4a",
        "aigc_str.mp4",
        "avc_aac.mp4",
        "avcc_10sec.mp4",
        "avcc_aac_mp3.mp4",
        "camera.mp4",
        "camera_h265_aac_rotate270.mp4",
        "camera_info_parser.mp4",
        "comment_0010.mp4",
        "comment_0020.mp4",
        "comment_0030.mp4",
        "comment_0040.mp4",
        "comment_0050.mp4",
        "comment_0060.mp4",
        "demuxer_parser_2_layer_frame_avc.mp4",
        "demuxer_parser_2_layer_frame_hevc.mp4",
        "demuxer_parser_3_layer_frame_avc.mp4",
        "demuxer_parser_3_layer_frame_hevc.mp4",
        "demuxer_parser_4_layer_frame_avc.mp4",
        "demuxer_parser_4_layer_frame_hevc.mp4",
        "demuxer_parser_all_i_frame_avc.mp4",
        "demuxer_parser_all_i_frame_hevc.mp4",
        "demuxer_parser_hdr_1_hevc.mp4",
        "demuxer_parser_hdr_2_hevc.mp4",
        "demuxer_parser_hdr_vivid.mp4",
        "demuxer_parser_hdr_vivid_gltf.mp4",
        "demuxer_parser_ipb_frame_avc.mp4",
        "demuxer_parser_ipb_frame_hevc.mp4",
        "demuxer_parser_ltr_frame_avc.mp4",
        "demuxer_parser_ltr_frame_hevc.mp4",
        "demuxer_parser_one_i_frame_avc.mp4",
        "demuxer_parser_one_i_frame_hevc.mp4",
        "demuxer_parser_one_i_frame_no_audio_avc.mp4",
        "demuxer_parser_one_i_frame_no_audio_hevc.mp4",
        "demuxer_parser_recording.mp4",
        "demuxer_parser_sdtp_extended_hevc.mp4",
        "demuxer_parser_sdtp_frame_avc.mp4",
        "demuxer_parser_sdtp_frame_hevc.mp4",
        "double_hevc.mp4",
        "double_vivid.mp4",
        "FLIP_V.mp4",
        "FLIP_V_90.mp4",
        "gb2312.m4a",
        "gb2312.mp4",
        "gb18030.m4a",
        "gb18030.mp4",
        "gbk.m4a",
        "gbk.mp4",
        "h264.mp4",
        "h264_aac_ch5.1.mp4",
        "H264_AAC_multi_track.mp4",
        "h264_double_video_audio.mp4",
        "h265_mp3_gltf.mp4",
        "hdrType_hdr10.mp4",
        "hdrType_hdrVivid.mp4",
        "hdrType_hlg.mp4",
        "hdrvivid_720p_2s.mp4",
        "hevc_aac.mp4",
        "hlg.mp4",
        "hlgHdrVivid.mp4",
        "hoa3_16bit_48000Hz_640kbps.mp4",
        "hvcc.mp4",
        "large_tref.mp4",
        "loci_altitude_100.mp4",
        "loci_altitude_102.mp4",
        "loci_altitude_empty.mp4",
        "M4A_24K_2.m4a",
        "M4A_64K_2_main.m4a",
        "M4A_96K_1_cbr.m4a",
        "M4A_48000_1.m4a",
        "m4a_48000_1_ut.m4a",
        "M4A_48000_aigc.m4a",
        "m4a_m4a.m4a",
        "m4v_h264_noaudio.m4v",
        "m4v_hevc_aac.m4v",
        "metaAlltype.mp4",
        "metaFloatval.mp4",
        "metaIntval.mp4",
        "metaMaxval.mp4",
        "metaStringval.mp4",
        "metaval_01.m4a",
        "metaval_01.mp4",
        "metaval_03.m4a",
        "metaval_03.mp4",
        "metaval_04.m4a",
        "metaval_04.mp4",
        "metaval_05.m4a",
        "metaval_05.mp4",
        "mp3_h264.mp4",
        "mp3_h265.mp4",
        "mp3_h265_fmp4.mp4",
        "Muxer_Auxiliary.mp4",
        "Muxer_Auxiliary_02.mp4",
        "Muxer_Auxiliary_03.mp4",
        "Muxer_Auxiliary_04.mp4",
        "muxer_auxl_265.mp4",
        "muxer_auxl_265_264_aac.mp4",
        "Muxer_SetFormat_Comment_1000.mp4",
        "Muxer_SetFormat_Comment_1100.mp4",
        "Muxer_SetFormat_Comment_1200.mp4",
        "Muxer_SetFormat_Comment_1300.mp4",
        "noframe.mp4",
        "NoTimedmetadataAudio.mp4",
        "pqHdrvivid.mp4",
        "pre_record_parser.mp4",
        "ROTATE_270.mp4",
        "ROTATE_NONE.mp4",
        "sarnot1.mp4",
        "short_audio.mp4",
        "short_video.mp4",
        "single_60.mp4",
        "single_rk.mp4",
        "stereo_16bit_48000Hz_32kbps.mp4",
        "test_264_B_Gop25_4sec.mp4",
        "test_264_B_Gop25_4sec_cover.mp4",
        "test_264_B_Gop25_4sec_gltf.mp4",
        "test_265_B_Gop25_4sec.mp4",
        "test_h264.mp4",
        "test_starttime.mp4",
        "timed_metadata_track.mp4",
        "Timedmetadata1Track0.mp4",
        "Timedmetadata1Track1.mp4",
        "Timedmetadata1Track2.mp4",
        "Timedmetadata2Track2.mp4",
        "TimedmetadataAudio.mp4",
        "TimedmetadataVideo.mp4",
        "unsupport_type.mp4",
        "video_2audio.mp4",
        "video_9audio.mp4",
        "video_audio_one_frame_test.mp4",
        "vvc_1280_720_8.mp4",
        "vvc_1280_720_8_gltf.mp4",
        "webvtt.mp4",
        "wvtt.mp4",
        "xm.mp4",
        "zero_track.mp4",
    };
    auto iter = std::find_if(supportFiles.begin(), supportFiles.end(),
        [&fileName](std::string& file) {
            return fileName == file;
        });
    return (iter != supportFiles.end());
}

std::vector<std::string> CollectDemuxerResourceFiles()
{
    std::vector<std::string> filePaths;
    DIR* dir = opendir(TEST_DEMUXER_RESOURCE_DIR.c_str());
    if (dir == nullptr) {
        ADD_FAILURE() << "Failed to open resource dir: " << TEST_DEMUXER_RESOURCE_DIR;
        return filePaths;
    }

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;
        if (fileName == "." || fileName == ".." || !HasDemuxerResourceSuffix(fileName) || !IsSupportedFile(fileName)) {
            continue;
        }
        std::string filePath = TEST_DEMUXER_RESOURCE_DIR + "/" + fileName;
        struct stat fileStatus {};
        if (stat(filePath.c_str(), &fileStatus) != 0 || !S_ISREG(fileStatus.st_mode)) {
            continue;
        }
        filePaths.emplace_back(filePath);
    }
    closedir(dir);
    std::sort(filePaths.begin(), filePaths.end());
    return filePaths;
}

// FFmpeg may still convert HEVC hvcC to AnnexB through hevc_mp4toannexb when the HEVC parser
// library is unavailable, while MPEG4 demuxer now exposes the original codec config in that case.
static const std::vector<KnownSetDataSourceDiffResource> g_knownSetDataSourceDiffResources = {
    {TEST_DEMUXER_RESOURCE_DIR + "/265_pcm_s16le.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_16bit_44100Hz_64kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_44100Hz_16bit_32k.m4a",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_44100Hz_16bit_32k.mp4", {"outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_44100Hz_16bit_32k_noftyp.mp4", {"outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_16bit_32000Hz_704kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_16bit_96000Hz_704kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_24bit_48000Hz_704kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout", "bitsPerCodedSmp"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_4obj_16bit_48000Hz_1232kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/7_1_4_16bit_48000Hz_832kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/aac_hdrvivid.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/aigc_str.mov", {"title"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/audiovivid_hdrvivid.mp4", {"csd", "outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/audiovivid_hdrvivid_2s.mp4", {"csd", "outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/camera.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/camera_h265_aac_rotate270.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h264_double_video_audio.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h265_fmp4.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h265_opus_ch7.1.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h265_pcm_s16le.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_db_no_image.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_hdr10.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_hdrVivid.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_hlg.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_sdr.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrvivid_720p_2s.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hevc_aac.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hevc_aac.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hoa3_16bit_48000Hz_640kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/multi_trk.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/muxer_auxl_265.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/noframe.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/stereo_16bit_48000Hz_32kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/test_265_B_Gop25_4sec.mp4", {"csd"}},
};

const std::vector<KnownSetDataSourceDiffResource>& GetKnownSetDataSourceDiffResources()
{
    return g_knownSetDataSourceDiffResources;
}

std::vector<std::string> GetKnownIssueFields(const std::string& filePath)
{
    // These fields are known long-standing FFmpeg/MPEG4 metadata differences and should not
    // block the resource-wide compatibility guard.
    static const std::vector<std::string> commonIssueFields = {
        "timeScale",
        "encoder",
        "codecName",
        "blockAlign",
    };

    std::vector<std::string> issueFields = commonIssueFields;
    const auto& resources = GetKnownSetDataSourceDiffResources();
    auto iter = std::find_if(resources.begin(), resources.end(),
        [&filePath](const KnownSetDataSourceDiffResource& resource) {
            return resource.filePath == filePath;
        });
    if (iter == resources.end()) {
        return issueFields;
    }
    for (const auto& field : iter->issueFields) {
        if (!ContainsIssueField(issueFields, field)) {
            issueFields.emplace_back(field);
        }
    }
    return issueFields;
}

std::shared_ptr<DataSourceImpl> CreateFileDataSource(const std::string& filePath, int& fd)
{
    struct stat fileStatus {};
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        ADD_FAILURE() << "Failed to stat file: " << filePath;
        return nullptr;
    }
    fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        ADD_FAILURE() << "Failed to open file: " << filePath;
        return nullptr;
    }

    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    if (source->SetSource(mediaSource) != Status::OK) {
        ADD_FAILURE() << "Set source failed";
        return nullptr;
    }
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    if (streamDemuxer->Init(uri) != Status::OK) {
        ADD_FAILURE() << "Init stream demuxer failed";
        return nullptr;
    }
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    return std::make_shared<DataSourceImpl>(streamDemuxer, 0);
}

SetDataSourceRunResult RunSetDataSourceOnly(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const std::string& filePath)
{
    SetDataSourceRunResult result;
    int fd = -1;
    auto dataSource = CreateFileDataSource(filePath, fd);
    if (dataSource == nullptr) {
        if (fd >= 0) {
            close(fd);
        }
        return result;
    }
    result.ret = plugin->SetDataSource(dataSource);
    if (result.ret == Status::OK) {
        result.mediaInfoRet = plugin->GetMediaInfo(result.mediaInfo);
        result.hasMediaInfo = result.mediaInfoRet == Status::OK;
    }
    plugin->Reset();
    close(fd);
    return result;
}

std::shared_ptr<Plugins::MPEG4::Sample> CreateMpeg4Sample(int32_t size, bool hasData)
{
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = 0;
    sample->dts = 0;
    sample->duration = 0;
    sample->flag = Plugins::MPEG4::Sample::NONE;
    sample->size = size;
    if (hasData && size > 0) {
        sample->data = std::make_unique<uint8_t[]>(size);
        for (int32_t index = 0; index < size; ++index) {
            sample->data[index] = static_cast<uint8_t>(index);
        }
    }
    return sample;
}

struct FrameSampleDigest {
    int32_t actualSize = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t duration = 0;
    uint32_t flag = 0;
    std::vector<uint8_t> payload;
};

void WriteU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
{
    constexpr size_t byteIndex0 = 0;
    constexpr size_t byteIndex1 = 1;
    constexpr size_t byteIndex2 = 2;
    constexpr size_t byteIndex3 = 3;
    constexpr uint32_t byte0Shift = 24;
    constexpr uint32_t byte1Shift = 16;
    constexpr uint32_t byte2Shift = 8;

    data[offset + byteIndex0] = static_cast<uint8_t>((value >> byte0Shift) & 0xFF);
    data[offset + byteIndex1] = static_cast<uint8_t>((value >> byte1Shift) & 0xFF);
    data[offset + byteIndex2] = static_cast<uint8_t>((value >> byte2Shift) & 0xFF);
    data[offset + byteIndex3] = static_cast<uint8_t>(value & 0xFF);
}

std::shared_ptr<Plugins::MPEG4::MPEG4Sample> MakeWebVttCueSample(
    int64_t pts, int64_t duration, const std::string& payload)
{
    constexpr size_t boxHeaderSize = 8;
    constexpr size_t boxTypeOffset = sizeof(uint32_t);
    uint32_t payloadBoxSize = static_cast<uint32_t>(boxHeaderSize + payload.size());
    uint32_t cueBoxSize = static_cast<uint32_t>(boxHeaderSize + payloadBoxSize);
    std::vector<uint8_t> sampleData(cueBoxSize + 1, 0);
    WriteU32(sampleData, 0, cueBoxSize);
    WriteU32(sampleData, boxTypeOffset, Plugins::MPEG4::FourccType("vttc"));
    WriteU32(sampleData, boxHeaderSize, payloadBoxSize);
    WriteU32(sampleData, boxHeaderSize + boxTypeOffset, Plugins::MPEG4::FourccType("payl"));
    std::copy(payload.begin(), payload.end(), sampleData.begin() + boxHeaderSize + boxHeaderSize);

    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = pts;
    sample->dts = pts;
    sample->duration = duration;
    sample->flag = Plugins::MPEG4::Sample::SYNC_FRAME;
    sample->size = static_cast<int32_t>(cueBoxSize);
    sample->data = std::make_unique<uint8_t[]>(sampleData.size());
    std::copy(sampleData.begin(), sampleData.end(), sample->data.get());

    auto mpeg4Sample = std::make_shared<Plugins::MPEG4::MPEG4Sample>();
    mpeg4Sample->sample = sample;
    return mpeg4Sample;
}

std::shared_ptr<Plugins::MPEG4::Sample> MakePendingWebVttSample(int64_t pts, int64_t duration)
{
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = pts;
    sample->dts = pts;
    sample->duration = duration;
    sample->flag = Plugins::MPEG4::Sample::SYNC_FRAME;
    sample->size = 1;
    return sample;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> MakeWebVttTrack(uint32_t trackIndex, int64_t duration)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    track->trackIndex = trackIndex;
    track->duration = duration;
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    track->sampleHelper->mimeType_ = MimeType::TEXT_WEBVTT;
    return track;
}

uint32_t FindTrackByMime(const MediaInfo& mediaInfo, const std::string& expectedMime)
{
    for (uint32_t trackId = 0; trackId < mediaInfo.tracks.size(); ++trackId) {
        std::string mime;
        if (mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, mime) && mime == expectedMime) {
            return trackId;
        }
    }
    return INVALID_TRACK_ID;
}

uint32_t FindFirstVideoTrack(const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds)
{
    for (auto trackId : selectedTrackIds) {
        std::string mime;
        if (trackId < mediaInfo.tracks.size() &&
            mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, mime) && mime.find("video/") == 0) {
            return trackId;
        }
    }
    return INVALID_TRACK_ID;
}

const TrackReadDigest* FindTrackDigest(const ReadDigest& digest, uint32_t trackId)
{
    for (const auto& trackDigest : digest.tracks) {
        if (trackDigest.trackId == trackId) {
            return &trackDigest;
        }
    }
    return nullptr;
}

bool SelectSourceSyncSampleByPts(
    const TrackReadDigest& digest, int64_t targetPtsUs, SeekMode seekMode, size_t& sampleIndex)
{
    int64_t previousKeyIndex = -1;
    int64_t nextKeyIndex = -1;
    for (auto keyIndex : digest.keyFrameIndexes) {
        if (keyIndex >= digest.samplePts.size()) {
            return false;
        }
        int64_t keyPts = digest.samplePts[keyIndex];
        if (keyPts <= targetPtsUs) {
            previousKeyIndex = static_cast<int64_t>(keyIndex);
        }
        if (keyPts >= targetPtsUs && nextKeyIndex < 0) {
            nextKeyIndex = static_cast<int64_t>(keyIndex);
        }
    }

    int64_t selectedKeyIndex = -1;
    if (seekMode == SeekMode::SEEK_PREVIOUS_SYNC) {
        selectedKeyIndex = previousKeyIndex;
    } else if (seekMode == SeekMode::SEEK_NEXT_SYNC) {
        selectedKeyIndex = nextKeyIndex;
    } else if (previousKeyIndex >= 0 && nextKeyIndex >= 0) {
        int64_t previousDiff = std::abs(targetPtsUs - digest.samplePts[previousKeyIndex]);
        int64_t nextDiff = std::abs(digest.samplePts[nextKeyIndex] - targetPtsUs);
        selectedKeyIndex = previousDiff <= nextDiff ? previousKeyIndex : nextKeyIndex;
    } else {
        selectedKeyIndex = previousKeyIndex >= 0 ? previousKeyIndex : nextKeyIndex;
    }
    if (selectedKeyIndex < 0) {
        return false;
    }
    sampleIndex = static_cast<size_t>(selectedKeyIndex);
    return true;
}

bool CopySamplePayload(const std::shared_ptr<MediaAVBuffer>& buffer, FrameSampleDigest& sample)
{
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        ADD_FAILURE() << "Sample buffer memory is nullptr";
        return false;
    }
    sample.actualSize = buffer->memory_->GetSize();
    if (sample.actualSize <= 0 || buffer->memory_->GetAddr() == nullptr) {
        ADD_FAILURE() << "Invalid sample memory size: " << sample.actualSize;
        return false;
    }
    auto data = buffer->memory_->GetAddr();
    sample.payload.assign(data, data + sample.actualSize);
    return true;
}

template<typename ReadSampleFunc, typename IsEosFunc>
std::vector<FrameSampleDigest> ReadTrackSamples(const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
    ReadSampleFunc readSample, IsEosFunc isEos)
{
    std::vector<FrameSampleDigest> samples;
    for (size_t sampleCount = 0; sampleCount < MAX_WEBVTT_SAMPLE_READ_COUNT; ++sampleCount) {
        AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        auto ret = readSample(plugin, trackId, buffer.mediaAVBuffer);
        if (ret == Status::END_OF_STREAM || isEos(buffer.mediaAVBuffer)) {
            return samples;
        }
        if (!(ret == Status::OK || ret == Status::NO_ERROR)) {
            ADD_FAILURE() << "ReadSample failed, trackId=" << trackId
                          << ", ret=" << static_cast<int32_t>(ret);
            return samples;
        }
        FrameSampleDigest sample;
        sample.pts = buffer.mediaAVBuffer->pts_;
        sample.dts = buffer.mediaAVBuffer->dts_;
        sample.duration = buffer.mediaAVBuffer->duration_;
        sample.flag = buffer.mediaAVBuffer->flag_;
        if (!CopySamplePayload(buffer.mediaAVBuffer, sample)) {
            return samples;
        }
        samples.push_back(std::move(sample));
    }
    ADD_FAILURE() << "ReadSample exceeded max WebVTT sample count, trackId=" << trackId;
    return samples;
}

void CompareWebVttFrameSamples(const std::vector<FrameSampleDigest>& actual,
    const std::vector<FrameSampleDigest>& expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(actual[index].actualSize, expected[index].actualSize);
        EXPECT_EQ(actual[index].pts, expected[index].pts);
        EXPECT_EQ(actual[index].dts, expected[index].dts);
        if (index + 1 == expected.size()) {
            EXPECT_GT(actual[index].duration, 0);
            EXPECT_LE(actual[index].duration, expected[index].duration);
        } else {
            EXPECT_EQ(actual[index].duration, expected[index].duration);
        }
        EXPECT_EQ(actual[index].flag, expected[index].flag);
        EXPECT_EQ(actual[index].payload, expected[index].payload);
    }
}

}

namespace OHOS {
namespace Media {
void Mpeg4DemuxerPluginUnitTest::SetUpTestCase()
{
    if (g_server != nullptr) {
        g_server->StopServer();
        g_server.reset();
    }
    g_server = std::make_unique<FileServerDemo>();
    g_server->StartServer();
}

void Mpeg4DemuxerPluginUnitTest::TearDownTestCase()
{
    if (g_server == nullptr) {
        return;
    }
    g_server->StopServer();
    g_server.reset();
}

void Mpeg4DemuxerPluginUnitTest::TearDown()
{
    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
}

void Mpeg4DemuxerPluginUnitTest::ResetPlugin(std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd)
{
    if (plugin != nullptr) {
        plugin->Reset();
        plugin = nullptr;
    }
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

void Mpeg4DemuxerPluginUnitTest::InitPlugin(
    const std::string& filePath, const std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd, MediaInfo& mediaInfo)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);

    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(plugin, nullptr);
    ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitPluginURI(const std::string& filePath,
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, MediaInfo& mediaInfo)
{
    ASSERT_EQ(filePath.find(TEST_RELATIVE_PATH), 0U);
    auto uri = TEST_URI_PATH + filePath.substr(TEST_RELATIVE_PATH.size());
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(plugin, nullptr);
    ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkPlugin(
    const std::string& filePath, const WeakNetworkPluginContext& context, int64_t failOffset, size_t maxFailCount)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    context.fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(context.fd, 0);

    auto uri = "fd://" + std::to_string(context.fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    auto cb = std::make_shared<Mpeg4SourceCallback>(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(cb);
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    std::shared_ptr<BaseStreamDemuxer> streamDemuxer =
        std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(failOffset, maxFailCount);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(context.plugin, nullptr);
    ASSERT_EQ(context.plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(context.plugin->GetMediaInfo(context.mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitFfmpegPlugin(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    InitPlugin(filePath, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitFfmpegPluginURI(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    InitPluginURI(filePath, ffmpegPlugin_, ffmpegMediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitMpeg4Plugin(const std::string& filePath)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    InitPlugin(filePath, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitMpeg4PluginURI(const std::string& filePath)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    InitPluginURI(filePath, mpeg4Plugin_, mpeg4MediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkFfmpegPlugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    WeakNetworkPluginContext context {ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_};
    InitWeakNetworkPlugin(filePath, context, failOffset, maxFailCount);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkMpeg4Plugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    WeakNetworkPluginContext context {mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_};
    InitWeakNetworkPlugin(filePath, context, failOffset, maxFailCount);
}

void Mpeg4DemuxerPluginUnitTest::InitDeferredWeakNetworkMpeg4Plugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount, uint32_t failDelayMs)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    mpeg4Fd_ = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(mpeg4Fd_, 0);

    auto uri = "fd://" + std::to_string(mpeg4Fd_) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    auto cb = std::make_shared<Mpeg4SourceCallback>(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(cb);
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 0);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    ASSERT_EQ(mpeg4Plugin_->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->GetMediaInfo(mpeg4MediaInfo_), Status::OK);
    streamDemuxer->ResetFailState();
    streamDemuxer->SetFailDelay(failDelayMs);
    streamDemuxer->SetFailParams(static_cast<size_t>(failOffset), maxFailCount);
}

bool Mpeg4DemuxerPluginUnitTest::IsSupportedTrack(const Meta& meta) const
{
    Plugins::MediaType trackType = Plugins::MediaType::UNKNOWN;
    if (!meta.Get<Tag::MEDIA_TYPE>(trackType)) {
        return false;
    }
    std::vector<uint8_t> cover;
    if (meta.Get<Tag::MEDIA_COVER>(cover)) {
        return false;
    }
    std::string mime;
    if (meta.Get<Tag::MIME_TYPE>(mime) && mime.find("image/") == 0) {
        return false;
    }
    return trackType == Plugins::MediaType::VIDEO || trackType == Plugins::MediaType::AUDIO ||
        trackType == Plugins::MediaType::SUBTITLE || trackType == Plugins::MediaType::TIMEDMETA;
}

bool Mpeg4DemuxerPluginUnitTest::IsEos(const std::shared_ptr<MediaAVBuffer>& buffer) const
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) != 0;
}

bool Mpeg4DemuxerPluginUnitTest::IsSyncFrame(const std::shared_ptr<MediaAVBuffer>& buffer) const
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0;
}

std::vector<uint32_t> Mpeg4DemuxerPluginUnitTest::SelectSupportedTracks(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, const MediaInfo& mediaInfo)
{
    std::vector<uint32_t> selectedTrackIds;
    for (uint32_t trackId = 0; trackId < mediaInfo.tracks.size(); ++trackId) {
        if (!IsSupportedTrack(mediaInfo.tracks[trackId])) {
            continue;
        }
        auto ret = plugin->SelectTrack(trackId);
        EXPECT_EQ(ret, Status::OK);
        if (ret == Status::OK) {
            selectedTrackIds.push_back(trackId);
        }
    }
    EXPECT_FALSE(selectedTrackIds.empty());
    return selectedTrackIds;
}

Status Mpeg4DemuxerPluginUnitTest::ReadSampleWithRetry(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
    const std::shared_ptr<MediaAVBuffer>& buffer, ReadMode readMode)
{
    Status ret = Status::OK;
    for (int32_t retry = 0; retry < WAIT_RETRY_TIMES; ++retry) {
        ret = (readMode == ReadMode::ZERO_COPY) ? plugin->ReadSampleZeroCopy(trackId, buffer,
            MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT) : plugin->ReadSample(trackId, buffer,
            MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        if (ret != Status::ERROR_WAIT_TIMEOUT) {
            return ret;
        }
    }
    return ret;
}

Status Mpeg4DemuxerPluginUnitTest::GetNextSampleSizeWithRetry(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    uint32_t trackId, int32_t& sampleSize)
{
    Status ret = Status::OK;
    for (int32_t retry = 0; retry < WAIT_RETRY_TIMES; ++retry) {
        ret = plugin->GetNextSampleSize(trackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        if (ret != Status::ERROR_WAIT_TIMEOUT) {
            return ret;
        }
    }
    return ret;
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekToFrameByDtsResult(
    const std::string& filePath, const std::string& testLabel)
{
    InitMpeg4Plugin(filePath);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    uint32_t videoTrackId = selectedTrackIds.front();
    bool hasVideoTrack = false;
    for (auto trackId : selectedTrackIds) {
        std::string mime;
        if (mpeg4MediaInfo_.tracks[trackId].Get<Tag::MIME_TYPE>(mime) && mime.find("video/") == 0) {
            videoTrackId = trackId;
            hasVideoTrack = true;
            break;
        }
    }
    ASSERT_TRUE(hasVideoTrack);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    std::vector<int64_t> targetDtsUsList;
    constexpr uint32_t readFrameCount = 40;
    constexpr uint32_t targetDtsCount = 3;
    constexpr int64_t usPerMs = 1000;
    for (uint32_t i = 0; i < readFrameCount; ++i) {
        ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, videoTrackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
        int64_t dtsUs = buffer.mediaAVBuffer->dts_;
        if (dtsUs % usPerMs != 0 ||
            std::find(targetDtsUsList.begin(), targetDtsUsList.end(), dtsUs) != targetDtsUsList.end()) {
            continue;
        }
        targetDtsUsList.push_back(dtsUs);
        std::cout << "[" << testLabel << "] source frame index: " << i <<
            ", source dts(us): " << dtsUs << std::endl;
        if (targetDtsUsList.size() == targetDtsCount) {
            break;
        }
    }
    ASSERT_EQ(targetDtsUsList.size(), targetDtsCount);

    for (auto targetDtsUs : targetDtsUsList) {
        int64_t seekTimeMs = targetDtsUs / usPerMs;
        int64_t realSeekTime = 0;
        ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(videoTrackId), seekTimeMs,
            SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
        EXPECT_EQ(realSeekTime, seekTimeMs * 1000000LL);

        ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, videoTrackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
        std::cout << "[" << testLabel << "] target dts(us): " << targetDtsUs
            << ", input seek dts(ms): " << seekTimeMs
            << ", real seek time(ns): " << realSeekTime
            << ", seeked frame dts(us): " << buffer.mediaAVBuffer->dts_ << std::endl;
        EXPECT_EQ(buffer.mediaAVBuffer->dts_, targetDtsUs);
    }
}

void Mpeg4DemuxerPluginUnitTest::InitPluginsForReadTest(const std::string& filePath, bool useUri)
{
    if (useUri) {
        InitFfmpegPluginURI(filePath);
        InitMpeg4PluginURI(filePath);
        return;
    }
    InitFfmpegPlugin(filePath);
    InitMpeg4Plugin(filePath);
}

SampleDigest Mpeg4DemuxerPluginUnitTest::ReadFirstSample(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const MediaInfo& mediaInfo, ReadMode readMode)
{
    SampleDigest digest;
    auto selectedTrackIds = SelectSupportedTracks(plugin, mediaInfo);
    if (selectedTrackIds.empty()) {
        ADD_FAILURE() << "No supported track selected";
        return digest;
    }

    digest.trackId = selectedTrackIds.front();
    mediaInfo.tracks[digest.trackId].GetData(Tag::MIME_TYPE, digest.mime);

    auto sampleSizeRet = GetNextSampleSizeWithRetry(plugin, digest.trackId, digest.nextSampleSize);
    EXPECT_EQ(sampleSizeRet, Status::OK);
    if (sampleSizeRet != Status::OK) {
        return digest;
    }
    EXPECT_GT(digest.nextSampleSize, 0);

    std::shared_ptr<MediaAVBuffer> buffer;
    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::ZERO_COPY) {
        buffer = MediaAVBuffer::CreateAVBuffer();
    } else {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        buffer = copyBuffer->mediaAVBuffer;
    }
    if (buffer == nullptr) {
        ADD_FAILURE() << "Create AVBuffer failed";
        return digest;
    }
    auto readRet = ReadSampleWithRetry(plugin, digest.trackId, buffer, readMode);
    EXPECT_EQ(readRet, Status::OK);
    if (readRet != Status::OK) {
        return digest;
    }
    EXPECT_FALSE(IsEos(buffer));

    digest.actualSize = buffer->memory_ != nullptr ? buffer->memory_->GetSize() : 0;
    digest.pts = buffer->pts_;
    digest.dts = buffer->dts_;
    digest.duration = buffer->duration_;
    digest.flag = buffer->flag_;
    EXPECT_GT(digest.actualSize, 0);
    return digest;
}

ReadDigest Mpeg4DemuxerPluginUnitTest::ReadAllSamples(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds, ReadMode readMode)
{
    auto digest = CreateReadDigest(mediaInfo, selectedTrackIds);
    std::vector<bool> eosFlags(selectedTrackIds.size(), false);
    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::COPY) {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    }

    while (std::find(eosFlags.begin(), eosFlags.end(), false) != eosFlags.end()) {
        for (size_t index = 0; index < selectedTrackIds.size(); ++index) {
            if (eosFlags[index]) {
                continue;
            }
            std::shared_ptr<MediaAVBuffer> buffer =
                (readMode == ReadMode::ZERO_COPY) ? MediaAVBuffer::CreateAVBuffer() : copyBuffer->mediaAVBuffer;
            bool eosReached = false;
            TrackSampleReadContext context {plugin, digest.tracks[index], buffer, readMode};
            if (!ReadTrackSample(context, eosReached)) {
                return digest;
            }
            eosFlags[index] = eosReached;
        }
    }
    return digest;
}

ReadDigest Mpeg4DemuxerPluginUnitTest::CreateReadDigest(
    const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds) const
{
    ReadDigest digest;
    for (auto trackId : selectedTrackIds) {
        TrackReadDigest trackDigest;
        trackDigest.trackId = trackId;
        mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, trackDigest.mime);
        digest.tracks.push_back(trackDigest);
    }
    return digest;
}

bool Mpeg4DemuxerPluginUnitTest::ReadTrackSample(const TrackSampleReadContext& context, bool& eosReached)
{
    if (context.buffer == nullptr) {
        ADD_FAILURE() << "Create AVBuffer failed";
        return false;
    }
    auto& trackDigest = context.trackDigest;
    auto ret = ReadSampleWithRetry(context.plugin, trackDigest.trackId, context.buffer, context.readMode);
    if (!(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR)) {
        ADD_FAILURE() << "ReadSample failed, trackId=" << trackDigest.trackId
                      << ", ret=" << static_cast<int32_t>(ret);
        return false;
    }
    eosReached = ret == Status::END_OF_STREAM || IsEos(context.buffer);
    if (eosReached) {
        return true;
    }
    if (trackDigest.frameCount == 0) {
        trackDigest.firstPts = context.buffer->pts_;
    }
    trackDigest.lastPts = context.buffer->pts_;
    if (IsSyncFrame(context.buffer)) {
        trackDigest.keyFrameCount++;
        trackDigest.keyFrameIndexes.push_back(static_cast<uint32_t>(trackDigest.frameCount));
    }
    if ((context.buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::DISCARD)) != 0) {
        trackDigest.discardFrameCount++;
        trackDigest.discardFrameIndexes.push_back(static_cast<uint32_t>(trackDigest.frameCount));
    }
    trackDigest.samplePts.push_back(context.buffer->pts_);
    trackDigest.sampleDts.push_back(context.buffer->dts_);
    trackDigest.sampleDurations.push_back(context.buffer->duration_);
    trackDigest.sampleDiscardFlags.push_back(
        context.buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::DISCARD));
    std::vector<uint8_t> skipInfo;
    if (context.buffer->meta_ != nullptr &&
        context.buffer->meta_->GetData(Tag::BUFFER_SKIP_SAMPLES_INFO, skipInfo)) {
        trackDigest.sampleSkipInfos.push_back(skipInfo);
    } else {
        trackDigest.sampleSkipInfos.emplace_back();
    }
    trackDigest.frameCount++;
    return true;
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekSuccessDigest(const ReadDigest& actualDigest,
    const ReadDigest& expectedDigest)
{
    ASSERT_EQ(actualDigest.tracks.size(), expectedDigest.tracks.size());
    for (size_t index = 0; index < actualDigest.tracks.size(); ++index) {
        EXPECT_EQ(actualDigest.tracks[index].mime, expectedDigest.tracks[index].mime);
        EXPECT_GT(actualDigest.tracks[index].frameCount, 0);
        EXPECT_LE(actualDigest.tracks[index].frameCount, expectedDigest.tracks[index].frameCount);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekSuccessDigestWithExpected(const ReadDigest& ffmpegDigest,
    const ReadDigest& mpeg4Digest, const std::vector<int32_t>& expectedFrames)
{
    ASSERT_EQ(ffmpegDigest.tracks.size(), expectedFrames.size());
    ASSERT_EQ(mpeg4Digest.tracks.size(), expectedFrames.size());
    for (size_t index = 0; index < expectedFrames.size(); ++index) {
        EXPECT_EQ(mpeg4Digest.tracks[index].mime, ffmpegDigest.tracks[index].mime);
        int32_t mpeg4DisplayFrameCount = mpeg4Digest.tracks[index].frameCount -
            mpeg4Digest.tracks[index].discardFrameCount;
        EXPECT_GT(mpeg4DisplayFrameCount, 0);
        EXPECT_LE(mpeg4DisplayFrameCount, expectedFrames[index]);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekFallbackDigest(const ReadDigest& mpeg4Digest,
    const std::vector<uint32_t>& selectedTrackIds, const std::vector<int32_t>& expectedFrames)
{
    if (expectedFrames.empty()) {
        EXPECT_EQ(mpeg4Digest.tracks.size(), selectedTrackIds.size());
        return;
    }
    ASSERT_EQ(mpeg4Digest.tracks.size(), expectedFrames.size());
    for (size_t index = 0; index < expectedFrames.size(); ++index) {
        int32_t mpeg4DisplayFrameCount = mpeg4Digest.tracks[index].frameCount -
            mpeg4Digest.tracks[index].discardFrameCount;
        EXPECT_GT(mpeg4DisplayFrameCount, 0);
        EXPECT_LE(mpeg4DisplayFrameCount, expectedFrames[index]);
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareReadDigest(const ReadDigest& lhs, const ReadDigest& rhs)
{
    ASSERT_EQ(lhs.tracks.size(), rhs.tracks.size());
    for (size_t index = 0; index < lhs.tracks.size(); ++index) {
        const auto& lhsTrack = lhs.tracks[index];
        const auto& rhsTrack = rhs.tracks[index];
        EXPECT_EQ(lhsTrack.mime, rhsTrack.mime);
        EXPECT_EQ(lhsTrack.frameCount, rhsTrack.frameCount);
        EXPECT_EQ(lhsTrack.keyFrameCount, rhsTrack.keyFrameCount);
        constexpr int64_t timestampToleranceUs = 100;
        EXPECT_NEAR(lhsTrack.firstPts, rhsTrack.firstPts, timestampToleranceUs);
        EXPECT_NEAR(lhsTrack.lastPts, rhsTrack.lastPts, timestampToleranceUs);
        EXPECT_EQ(lhsTrack.keyFrameIndexes, rhsTrack.keyFrameIndexes);
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareTrackPropertiesWithFfmpeg(
    const std::vector<uint32_t>& mpeg4SelectedTrackIds, const std::vector<uint32_t>& ffmpegSelectedTrackIds,
    bool compareBitRate)
{
    ASSERT_EQ(mpeg4SelectedTrackIds.size(), ffmpegSelectedTrackIds.size());
    for (size_t index = 0; index < mpeg4SelectedTrackIds.size(); ++index) {
        SCOPED_TRACE("track index=" + std::to_string(index));
        const auto mpeg4TrackId = mpeg4SelectedTrackIds[index];
        const auto ffmpegTrackId = ffmpegSelectedTrackIds[index];
        ASSERT_LT(mpeg4TrackId, mpeg4MediaInfo_.tracks.size());
        ASSERT_LT(ffmpegTrackId, ffmpegMediaInfo_.tracks.size());
        const auto& mpeg4Track = mpeg4MediaInfo_.tracks[mpeg4TrackId];
        const auto& ffmpegTrack = ffmpegMediaInfo_.tracks[ffmpegTrackId];

        std::string mpeg4Mime;
        std::string ffmpegMime;
        EXPECT_EQ(mpeg4Track.Get<Tag::MIME_TYPE>(mpeg4Mime), ffmpegTrack.Get<Tag::MIME_TYPE>(ffmpegMime));
        EXPECT_EQ(mpeg4Mime, ffmpegMime);

        Plugins::MediaType mpeg4Type = Plugins::MediaType::UNKNOWN;
        Plugins::MediaType ffmpegType = Plugins::MediaType::UNKNOWN;
        EXPECT_EQ(mpeg4Track.Get<Tag::MEDIA_TYPE>(mpeg4Type), ffmpegTrack.Get<Tag::MEDIA_TYPE>(ffmpegType));
        EXPECT_EQ(mpeg4Type, ffmpegType);

        if (compareBitRate) {
            int64_t mpeg4BitRate = 0;
            int64_t ffmpegBitRate = 0;
            bool hasMpeg4BitRate = mpeg4Track.Get<Tag::MEDIA_BITRATE>(mpeg4BitRate);
            bool hasFfmpegBitRate = ffmpegTrack.Get<Tag::MEDIA_BITRATE>(ffmpegBitRate);
            EXPECT_EQ(hasMpeg4BitRate, hasFfmpegBitRate);
            if (hasMpeg4BitRate && hasFfmpegBitRate) {
                EXPECT_EQ(mpeg4BitRate, ffmpegBitRate);
            }
        }

        int64_t mpeg4StartTime = 0;
        int64_t ffmpegStartTime = 0;
        bool hasMpeg4StartTime = mpeg4Track.Get<Tag::MEDIA_START_TIME>(mpeg4StartTime);
        bool hasFfmpegStartTime = ffmpegTrack.Get<Tag::MEDIA_START_TIME>(ffmpegStartTime);
        EXPECT_EQ(hasMpeg4StartTime, hasFfmpegStartTime);
        if (hasMpeg4StartTime && hasFfmpegStartTime) {
            EXPECT_EQ(mpeg4StartTime, ffmpegStartTime);
        }

        if (mpeg4Type == Plugins::MediaType::VIDEO || ffmpegType == Plugins::MediaType::VIDEO) {
            int32_t mpeg4VideoDelay = 0;
            int32_t ffmpegVideoDelay = 0;
            bool hasMpeg4VideoDelay = mpeg4Track.Get<Tag::VIDEO_DELAY>(mpeg4VideoDelay);
            bool hasFfmpegVideoDelay = ffmpegTrack.Get<Tag::VIDEO_DELAY>(ffmpegVideoDelay);
            EXPECT_EQ(hasMpeg4VideoDelay, hasFfmpegVideoDelay);
            if (hasMpeg4VideoDelay && hasFfmpegVideoDelay) {
                EXPECT_EQ(mpeg4VideoDelay, ffmpegVideoDelay);
            }
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareTimelineDigest(const ReadDigest& mpeg4Digest,
    const ReadDigest& ffmpegDigest, bool compareFullTimeline, bool compareDurations)
{
    ASSERT_EQ(mpeg4Digest.tracks.size(), ffmpegDigest.tracks.size());
    for (size_t index = 0; index < mpeg4Digest.tracks.size(); ++index) {
        SCOPED_TRACE("track index=" + std::to_string(index));
        const auto& mpeg4Track = mpeg4Digest.tracks[index];
        const auto& ffmpegTrack = ffmpegDigest.tracks[index];
        size_t compareCount = compareFullTimeline ? ffmpegTrack.samplePts.size() :
            std::min(mpeg4Track.samplePts.size(), ffmpegTrack.samplePts.size());
        ASSERT_GE(mpeg4Track.samplePts.size(), compareCount);
        ASSERT_GE(ffmpegTrack.samplePts.size(), compareCount);
        EXPECT_EQ(mpeg4Track.mime, ffmpegTrack.mime);
        EXPECT_EQ(std::vector<int64_t>(mpeg4Track.samplePts.begin(), mpeg4Track.samplePts.begin() + compareCount),
            std::vector<int64_t>(ffmpegTrack.samplePts.begin(), ffmpegTrack.samplePts.begin() + compareCount));
        EXPECT_EQ(std::vector<int64_t>(mpeg4Track.sampleDts.begin(), mpeg4Track.sampleDts.begin() + compareCount),
            std::vector<int64_t>(ffmpegTrack.sampleDts.begin(), ffmpegTrack.sampleDts.begin() + compareCount));
        if (compareDurations) {
            EXPECT_EQ(std::vector<int64_t>(mpeg4Track.sampleDurations.begin(),
                mpeg4Track.sampleDurations.begin() + compareCount),
                std::vector<int64_t>(ffmpegTrack.sampleDurations.begin(),
                    ffmpegTrack.sampleDurations.begin() + compareCount));
        }
        if (compareFullTimeline) {
            EXPECT_EQ(mpeg4Track.discardFrameCount, ffmpegTrack.discardFrameCount);
            EXPECT_EQ(mpeg4Track.discardFrameIndexes, ffmpegTrack.discardFrameIndexes);
        }
        EXPECT_EQ(std::vector<uint32_t>(mpeg4Track.sampleDiscardFlags.begin(),
            mpeg4Track.sampleDiscardFlags.begin() + compareCount),
            std::vector<uint32_t>(ffmpegTrack.sampleDiscardFlags.begin(),
                ffmpegTrack.sampleDiscardFlags.begin() + compareCount));
        EXPECT_EQ(std::vector<std::vector<uint8_t>>(mpeg4Track.sampleSkipInfos.begin(),
            mpeg4Track.sampleSkipInfos.begin() + compareCount),
            std::vector<std::vector<uint8_t>>(ffmpegTrack.sampleSkipInfos.begin(),
                ffmpegTrack.sampleSkipInfos.begin() + compareCount));
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareMovFixIndexResultWithFfmpeg(const std::string& filePath,
    bool compareBitRate, bool compareFullTimeline, bool compareDurations)
{
    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    SCOPED_TRACE(filePath);
    InitFfmpegPlugin(filePath);
    InitMpeg4Plugin(filePath);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(mpeg4SelectedTracks.size(), ffmpegSelectedTracks.size());
    CompareTrackPropertiesWithFfmpeg(mpeg4SelectedTracks, ffmpegSelectedTracks, compareBitRate);

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    if (compareFullTimeline) {
        CompareReadDigest(mpeg4Digest, ffmpegDigest);
    } else {
        ASSERT_FALSE(mpeg4Digest.tracks.empty());
        for (const auto& trackDigest : mpeg4Digest.tracks) {
            EXPECT_GT(trackDigest.frameCount, 0);
        }
    }
    CompareTimelineDigest(mpeg4Digest, ffmpegDigest, compareFullTimeline, compareDurations);

    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
}

void Mpeg4DemuxerPluginUnitTest::CheckReadResultWithExpected(const std::string& filePath,
    const std::vector<int32_t>& expectedFrames, const std::vector<int32_t>& expectedKeyFrames,
    const std::vector<uint32_t>& expectedKeyFrameIndexes, bool useUri)
{
    if (useUri) {
        InitMpeg4PluginURI(filePath);
    } else {
        InitMpeg4Plugin(filePath);
    }
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, selectedTrackIds);
    CheckReadDigestWithExpected(digest, expectedFrames, expectedKeyFrames, expectedKeyFrameIndexes);
}

void Mpeg4DemuxerPluginUnitTest::CheckReadDigestWithExpected(const ReadDigest& digest,
    const std::vector<int32_t>& expectedFrames, const std::vector<int32_t>& expectedKeyFrames,
    const std::vector<uint32_t>& expectedKeyFrameIndexes)
{
    ASSERT_EQ(digest.tracks.size(), expectedFrames.size());
    ASSERT_EQ(digest.tracks.size(), expectedKeyFrames.size());
    for (size_t index = 0; index < digest.tracks.size(); ++index) {
        EXPECT_EQ(digest.tracks[index].frameCount, expectedFrames[index]);
        EXPECT_EQ(digest.tracks[index].keyFrameCount, expectedKeyFrames[index]);
        if (index == 0) {
            EXPECT_EQ(digest.tracks[index].keyFrameIndexes, expectedKeyFrameIndexes);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareSampleDigest(const SampleDigest& lhs, const SampleDigest& rhs)
{
    EXPECT_EQ(lhs.mime, rhs.mime);
    if (lhs.mime.find("video/") == 0 &&
        (lhs.flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0) {
        // FFmpeg prepends codec config on every sync frame while MPEG4Demuxer only prepends it on the first one.
        // The zero-copy test validates decodable sample attributes rather than requiring identical advertised size.
        EXPECT_GT(lhs.nextSampleSize, 0);
        EXPECT_GT(rhs.nextSampleSize, 0);
        EXPECT_LE(lhs.actualSize, rhs.nextSampleSize);
        EXPECT_LE(rhs.actualSize, lhs.nextSampleSize);
    } else {
        EXPECT_EQ(lhs.nextSampleSize, rhs.nextSampleSize);
    }
    EXPECT_EQ(lhs.actualSize, rhs.actualSize);
    EXPECT_EQ(lhs.pts, rhs.pts);
    EXPECT_EQ(lhs.dts, rhs.dts);
    EXPECT_EQ(lhs.duration, rhs.duration);
    EXPECT_EQ(lhs.flag, rhs.flag);
}

void Mpeg4DemuxerPluginUnitTest::CompareSeekReadResult(
    const std::string& filePath, const std::vector<int64_t>& seekTimes,
    const std::vector<SeekMode>& seekModes, bool useUri)
{
    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            InitPluginsForReadTest(filePath, useUri);

            auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
            auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

            int64_t ffmpegRealSeekTime = 0;
            int64_t mpeg4RealSeekTime = 0;
            auto ffmpegRet = ffmpegPlugin_->SeekTo(-1, seekTimeTarget, seekMode, ffmpegRealSeekTime);
            auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, mpeg4RealSeekTime);
            ASSERT_EQ(mpeg4Ret, ffmpegRet);
            if (ffmpegRet != Status::OK) {
                ResetPlugin(ffmpegPlugin_, ffmpegFd_);
                ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
                continue;
            }
            auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekSuccessDigest(mpeg4Digest, ffmpegDigest);

            ResetPlugin(ffmpegPlugin_, ffmpegFd_);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekFirstVideoSampleWithSourceTimeline(
    const std::string& filePath, const std::vector<int64_t>& seekTimes,
    const std::vector<SeekMode>& seekModes, bool useUri)
{
    useUri ? InitMpeg4PluginURI(filePath) : InitMpeg4Plugin(filePath);
    auto sourceSelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    uint32_t sourceVideoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, sourceSelectedTracks);
    ASSERT_NE(sourceVideoTrackId, INVALID_TRACK_ID);
    int64_t videoStartTimeUs = 0;
    (void)mpeg4MediaInfo_.tracks[sourceVideoTrackId].Get<Tag::MEDIA_START_TIME>(videoStartTimeUs);
    auto sourceDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, sourceSelectedTracks);
    const auto* sourceVideoDigest = FindTrackDigest(sourceDigest, sourceVideoTrackId);
    ASSERT_NE(sourceVideoDigest, nullptr);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);

    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            SCOPED_TRACE("seekTime=" + std::to_string(seekTimeTarget) +
                ", mode=" + std::to_string(static_cast<int32_t>(seekMode)));
            int64_t targetPtsUs = videoStartTimeUs + seekTimeTarget * TEST_MS_TO_US;
            size_t expectedSampleIndex = 0;
            ASSERT_TRUE(SelectSourceSyncSampleByPts(*sourceVideoDigest, targetPtsUs, seekMode, expectedSampleIndex));

            useUri ? InitMpeg4PluginURI(filePath) : InitMpeg4Plugin(filePath);
            auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            uint32_t mpeg4VideoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, mpeg4SelectedTracks);
            ASSERT_NE(mpeg4VideoTrackId, INVALID_TRACK_ID);

            int64_t mpeg4RealSeekTime = 0;
            auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, mpeg4RealSeekTime);
            ASSERT_EQ(mpeg4Ret, Status::OK);
            EXPECT_EQ(mpeg4RealSeekTime, targetPtsUs * TEST_US_TO_NS);

            AVBufferWrapper mpeg4Buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
            ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, mpeg4VideoTrackId, mpeg4Buffer.mediaAVBuffer,
                ReadMode::COPY), Status::OK);
            EXPECT_FALSE(IsEos(mpeg4Buffer.mediaAVBuffer));
            EXPECT_EQ(mpeg4Buffer.mediaAVBuffer->pts_, sourceVideoDigest->samplePts[expectedSampleIndex]);
            EXPECT_EQ(mpeg4Buffer.mediaAVBuffer->dts_, sourceVideoDigest->sampleDts[expectedSampleIndex]);
            EXPECT_EQ(mpeg4Buffer.mediaAVBuffer->duration_, sourceVideoDigest->sampleDurations[expectedSampleIndex]);
            EXPECT_NE(mpeg4Buffer.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME), 0U);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekReadResultWithSourceTimeline(
    const std::string& filePath, const std::vector<int64_t>& seekTimes, const std::vector<SeekMode>& seekModes)
{
    InitMpeg4Plugin(filePath);
    auto sourceSelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    uint32_t sourceVideoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, sourceSelectedTracks);
    ASSERT_NE(sourceVideoTrackId, INVALID_TRACK_ID);
    int64_t videoStartTimeUs = 0;
    (void)mpeg4MediaInfo_.tracks[sourceVideoTrackId].Get<Tag::MEDIA_START_TIME>(videoStartTimeUs);
    auto sourceDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, sourceSelectedTracks);
    const auto* sourceVideoDigest = FindTrackDigest(sourceDigest, sourceVideoTrackId);
    ASSERT_NE(sourceVideoDigest, nullptr);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);

    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            SCOPED_TRACE("seekTime=" + std::to_string(seekTimeTarget) +
                ", mode=" + std::to_string(static_cast<int32_t>(seekMode)));
            size_t expectedSampleIndex = 0;
            int64_t targetPtsUs = videoStartTimeUs + seekTimeTarget * TEST_MS_TO_US;
            ASSERT_TRUE(SelectSourceSyncSampleByPts(*sourceVideoDigest, targetPtsUs, seekMode, expectedSampleIndex));
            InitMpeg4Plugin(filePath);
            auto selectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            int64_t realSeekTime = 0;
            ASSERT_EQ(mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, realSeekTime), Status::OK);
            EXPECT_EQ(realSeekTime, targetPtsUs * TEST_US_TO_NS);
            auto seekDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, selectedTracks);
            const auto* seekVideoDigest = FindTrackDigest(seekDigest, sourceVideoTrackId);
            ASSERT_NE(seekVideoDigest, nullptr);
            ASSERT_EQ(seekVideoDigest->samplePts.size(), sourceVideoDigest->samplePts.size() - expectedSampleIndex);
            EXPECT_TRUE(std::equal(sourceVideoDigest->samplePts.begin() + expectedSampleIndex,
                sourceVideoDigest->samplePts.end(), seekVideoDigest->samplePts.begin()));
            EXPECT_TRUE(std::equal(sourceVideoDigest->sampleDts.begin() + expectedSampleIndex,
                sourceVideoDigest->sampleDts.end(), seekVideoDigest->sampleDts.begin()));
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareWeakNetworkReadResult(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    InitWeakNetworkFfmpegPlugin(filePath, failOffset, maxFailCount);
    InitWeakNetworkMpeg4Plugin(filePath, failOffset, maxFailCount);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekReadResultWithExpected(const std::string& filePath,
    const std::vector<SeekExpectedResult>& expectedResults, bool useUri)
{
    for (const auto& expected : expectedResults) {
        InitPluginsForReadTest(filePath, useUri);

        auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
        auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
        ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

        int64_t ffmpegRealSeekTime = 0;
        int64_t mpeg4RealSeekTime = 0;
        auto ffmpegRet = ffmpegPlugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, ffmpegRealSeekTime);
        auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, mpeg4RealSeekTime);

        if (expected.seekSuccess) {
            ASSERT_EQ(ffmpegRet, Status::OK);
            ASSERT_EQ(mpeg4Ret, Status::OK);
            auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekSuccessDigestWithExpected(ffmpegDigest, mpeg4Digest, expected.expectedFrames);
            ResetPlugin(ffmpegPlugin_, ffmpegFd_);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
            continue;
        }
        EXPECT_NE(ffmpegRet, Status::OK);
        if (mpeg4Ret == Status::OK) {
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekFallbackDigest(mpeg4Digest, mpeg4SelectedTracks, expected.expectedFrames);
        }

        ResetPlugin(ffmpegPlugin_, ffmpegFd_);
        ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InitSelectTrack_001
 * @tc.desc: Directly create MPEG4DemuxerPlugin and select supported tracks
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InitSelectTrack_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    ASSERT_GT(mpeg4MediaInfo_.tracks.size(), 0U);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    EXPECT_FALSE(selectedTrackIds.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_RbspReadUEBoundary_001
 * @tc.desc: Verify unsigned Exp-Golomb boundary avoids 32-bit shift overflow
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_RbspReadUEBoundary_001, TestSize.Level1)
{
    const uint8_t maxLeadingZeroUe[] = {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00};
    RbspContext maxContext(maxLeadingZeroUe, static_cast<int32_t>(sizeof(maxLeadingZeroUe)));
    uint32_t value = 0;
    EXPECT_TRUE(maxContext.RbspReadUE(value));
    EXPECT_EQ(value, 0xFFFFFFFFU);

    RbspContext signedOverflowContext(maxLeadingZeroUe, static_cast<int32_t>(sizeof(maxLeadingZeroUe)));
    int32_t signedValue = 0;
    EXPECT_FALSE(signedOverflowContext.RbspReadSE(signedValue));

    const uint8_t overflowUe[] = {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80};
    RbspContext overflowContext(overflowUe, static_cast<int32_t>(sizeof(overflowUe)));
    value = 0;
    EXPECT_FALSE(overflowContext.RbspReadUE(value));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_RbspGetBitsBoundary_001
 * @tc.desc: Verify RbspGetBits handles zero, oversized and unaligned 32-bit reads safely
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_RbspGetBitsBoundary_001, TestSize.Level1)
{
    const uint8_t unalignedData[] = {0x0F, 0xFF, 0xFF, 0xFF, 0xF0};
    RbspContext unalignedContext(unalignedData, static_cast<int32_t>(sizeof(unalignedData)));
    EXPECT_EQ(static_cast<uint32_t>(unalignedContext.RbspGetBits<uint8_t>(4)), 0U);
    EXPECT_EQ((unalignedContext.RbspGetBits<uint32_t, uint64_t>(32)), 0xFFFFFFFFU);

    RbspContext invalidContext(unalignedData, static_cast<int32_t>(sizeof(unalignedData)));
    EXPECT_EQ((invalidContext.RbspGetBits<uint32_t, uint32_t>(0)), 0U);
    EXPECT_EQ(static_cast<uint32_t>(invalidContext.RbspGetBits<uint8_t, uint32_t>(9)), 0U);
    EXPECT_EQ((invalidContext.RbspGetBits<uint32_t, uint32_t>(33)), 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_LifecycleMetaAndStubInterfaces_001
 * @tc.desc: Verify MPEG4 demuxer lifecycle, meta, probe, drm and reference-parser interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_LifecycleMetaAndStubInterfaces_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);

    MediaInfo mediaInfo;
    EXPECT_EQ(plugin->GetMediaInfo(mediaInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->Start(), Status::OK);
    EXPECT_EQ(plugin->Stop(), Status::OK);
    EXPECT_EQ(plugin->Pause(), Status::OK);
    EXPECT_EQ(plugin->Flush(), Status::OK);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    plugin->ResetEosStatus();
    plugin->SetCacheLimit(1024);
    plugin->SetInterruptState(false);
    EXPECT_EQ(plugin->SetReadTimeoutForInitSeek(10), Status::OK);
    EXPECT_EQ(plugin->SetAVReadPacketStopState(false), Status::OK);
    EXPECT_EQ(plugin->SetCachePressureCallback(nullptr), Status::OK);
    EXPECT_EQ(plugin->SetTrackCacheLimit(0, 1024), Status::OK);

    int32_t offset = -1;
    int32_t size = -1;
    EXPECT_TRUE(plugin->GetProbeSize(offset, size));
    EXPECT_EQ(offset, 0);
    EXPECT_GT(size, 0);

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_EQ(plugin->GetDrmInfo(drmInfo), Status::OK);
    EXPECT_FALSE(plugin->IsRefParserSupported());
    EXPECT_EQ(plugin->ParserRefUpdatePos(0), Status::ERROR_UNSUPPORTED_FORMAT);
    EXPECT_EQ(plugin->ParserRefInfo(), Status::ERROR_UNKNOWN);

    auto buffer = MediaAVBuffer::CreateAVBuffer();
    FrameLayerInfo frameLayerInfo;
    GopLayerInfo gopLayerInfo;
    std::vector<uint32_t> iFramePos;
    uint32_t frameId = 0;
    int64_t seekMs = 0;
    EXPECT_EQ(plugin->GetFrameLayerInfo(buffer, frameLayerInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->GetFrameLayerInfo(frameId, frameLayerInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->GetGopLayerInfo(0, gopLayerInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->GetIFramePos(iFramePos), Status::ERROR_UNKNOWN);
    EXPECT_EQ(plugin->Dts2FrameId(0, frameId), Status::ERROR_OUT_OF_RANGE);
    EXPECT_EQ(plugin->SeekMs2FrameId(0, frameId), Status::ERROR_OUT_OF_RANGE);
    EXPECT_EQ(plugin->FrameId2SeekMs(frameId, seekMs), Status::ERROR_UNKNOWN);

    int64_t realSeekTime = 0;
    EXPECT_EQ(plugin->SeekToKeyFrame(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_001
 * @tc.desc: Verify MPEG4 demuxer cache pressure callback and per-track limit controls
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, 1), Status::OK);

    uint32_t initialFrameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, initialFrameCount), Status::OK);
    uint32_t initialCacheSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, initialCacheSize), Status::OK);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    EXPECT_EQ(ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);

    uint32_t frameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    uint32_t cacheSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, cacheSize), Status::OK);
    EXPECT_GT(callbackCount.load(), 0);
    EXPECT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 0), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_Disable_001
 * @tc.desc: Verify cache pressure notification stops after disabling per-track limit
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_Disable_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, 1), Status::OK);

    int64_t realSeekTime = 0;
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    ASSERT_GT(callbackCount.load(), 0);

    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 0), Status::OK);
    callbackCount.store(0);
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 40,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    EXPECT_EQ(callbackCount.load(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_Throttle_001
 * @tc.desc: Verify cache pressure callback is throttled by per-track notification window
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_Throttle_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    constexpr uint32_t throttleWindowMs = 10000;
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, throttleWindowMs), Status::OK);

    int64_t realSeekTime = 0;
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    int32_t firstCallbackCount = callbackCount.load();
    ASSERT_GT(firstCallbackCount, 0);

    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 40,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 80,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    EXPECT_EQ(callbackCount.load(), firstCallbackCount);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_AfterSeek_001
 * @tc.desc: Verify cache pressure and cache query remain valid after DTS seek
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_AfterSeek_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, 1), Status::OK);

    int64_t realSeekTime = 0;
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    EXPECT_GT(callbackCount.load(), 0);

    uint32_t frameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    EXPECT_GT(frameCount, 0);
    uint32_t cacheSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, cacheSize), Status::OK);
    EXPECT_GT(cacheSize, 0);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
    EXPECT_EQ(buffer.mediaAVBuffer->dts_, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToFrameByDts_001
 * @tc.desc: Verify MPEG4 demuxer can seek to the first frame whose DTS reaches the target
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToFrameByDts_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckSeekToFrameByDtsResult(TEST_MP4_PATH_H264_B, "SeekToFrameByDts_001"));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToFrameByDts_InvalidParam_001
 * @tc.desc: Verify MPEG4 demuxer SeekToFrameByDts parameter checks
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToFrameByDts_InvalidParam_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    int64_t realSeekTime = 0;
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_CLOSEST, realSeekTime, 0),
        Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_NULL_POINTER);

    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(0, 0, SeekMode::SEEK_CLOSEST, realSeekTime, 0),
        Status::ERROR_INVALID_OPERATION);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(-1, 0, SeekMode::SEEK_CLOSEST, realSeekTime, 0),
        Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutFail_001
 * @tc.desc: Verify MPEG4 demuxer init fails when ReadAt keeps returning ERROR_AGAIN beyond timeout
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutFail_001, TestSize.Level1)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(TEST_MP4_PATH_H264_B.c_str(), &fileStatus), 0);
    int fd = open(TEST_MP4_PATH_H264_B.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 1000);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_EQ(plugin->SetReadTimeoutForInitSeek(100), Status::OK);
    EXPECT_NE(plugin->SetDataSource(dataSource), Status::OK);
    close(fd);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutSuccess_001
 * @tc.desc: Verify MPEG4 demuxer init succeeds when read timeout is explicitly disabled
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutSuccess_001, TestSize.Level1)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(TEST_MP4_PATH_H264_B.c_str(), &fileStatus), 0);
    int fd = open(TEST_MP4_PATH_H264_B.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 20);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_EQ(plugin->SetReadTimeoutForInitSeek(0), Status::OK);
    EXPECT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    close(fd);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_001
 * @tc.desc: Verify MPEG4 demuxer DTS seek fails when prefetch read times out
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 1000);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(100), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_NE(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_002
 * @tc.desc: Verify SeekToFrameByDts timeout limits prefetch read even when internal read timeout is longer
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_002, TestSize.Level1)
{
    constexpr uint32_t apiTimeoutMs = 100;
    constexpr uint32_t internalTimeoutMs = 2000;
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 1000, 20);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(internalTimeoutMs), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_NE(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, apiTimeoutMs), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_001
 * @tc.desc: Verify MPEG4 demuxer DTS seek succeeds when read timeout is disabled
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 20);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(0), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, 0), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_002
 * @tc.desc: Verify MPEG4 demuxer DTS seek succeeds when read timeout is long enough
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_002, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 20);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(2000), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, 2000), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSourceWithProbSize_001
 * @tc.desc: Verify SetDataSourceWithProbSize parameter checks and normal initialization path
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSourceWithProbSize_001, TestSize.Level1)
{
    int fd = -1;
    auto dataSource = CreateFileDataSource(TEST_MP4_PATH_H264_B, fd);
    ASSERT_NE(dataSource, nullptr);

    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(nullptr, 0), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(dataSource, -1), Status::ERROR_INVALID_PARAMETER);
    auto setDataSourceWithProbeRet = plugin->SetDataSourceWithProbSize(dataSource, 1024);
    if (setDataSourceWithProbeRet != Status::OK) {
        EXPECT_EQ(setDataSourceWithProbeRet, Status::ERROR_INVALID_PARAMETER);
        ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    }

    MediaInfo mediaInfo;
    EXPECT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
    EXPECT_FALSE(mediaInfo.tracks.empty());
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(dataSource, 1024), Status::ERROR_WRONG_STATE);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    if (fd >= 0) {
        close(fd);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSource_AllDemuxerResources_001
 * @tc.desc: Verify FFmpeg and MPEG4 demuxer SetDataSource initialization coverage for mov/mp4 family resources
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSource_AllDemuxerResources_001, TestSize.Level1)
{
    auto filePaths = CollectDemuxerResourceFiles();
    ASSERT_FALSE(filePaths.empty());

    size_t ffmpegOkCount = 0;
    size_t mpeg4OkCount = 0;
    for (const auto& filePath : filePaths) {
        SCOPED_TRACE(filePath);
        auto ffmpegBasePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
        auto ffmpegPlugin = std::static_pointer_cast<BaseDemuxerPlugin>(
            std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(ffmpegBasePlugin));
        ASSERT_NE(ffmpegPlugin, nullptr);
        auto ffmpegResult = RunSetDataSourceOnly(ffmpegPlugin, filePath);
        if (ffmpegResult.ret == Status::OK) {
            ++ffmpegOkCount;
        }

        auto mpeg4Plugin = std::static_pointer_cast<BaseDemuxerPlugin>(
            std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
        ASSERT_NE(mpeg4Plugin, nullptr);
        auto mpeg4Result = RunSetDataSourceOnly(mpeg4Plugin, filePath);
        if (mpeg4Result.ret == Status::OK) {
            ++mpeg4OkCount;
        }
        auto ignoredFields = GetKnownIssueFields(filePath);
        ExpectSetDataSourceCompatible(filePath, ffmpegResult, mpeg4Result, ignoredFields);
    }

    EXPECT_GT(ffmpegOkCount, 0U);
    EXPECT_GT(mpeg4OkCount, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InvalidParameterAndTrackState_001
 * @tc.desc: Verify invalid parameters and select/unselect state handling
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InvalidParameterAndTrackState_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();
    auto invalidTrackId = static_cast<uint32_t>(mpeg4MediaInfo_.tracks.size());

    EXPECT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->SelectTrack(invalidTrackId), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->UnselectTrack(invalidTrackId), Status::ERROR_INVALID_PARAMETER);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    int32_t sampleSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(invalidTrackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(invalidTrackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, nullptr, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSampleZeroCopy(trackId, nullptr, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);

    ASSERT_EQ(mpeg4Plugin_->UnselectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->UnselectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SyncReadInterfaces_001
 * @tc.desc: Verify synchronous GetNextSampleSize and ReadSample interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SyncReadInterfaces_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();

    int32_t sampleSize = 0;
    ASSERT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize), Status::OK);
    EXPECT_GT(sampleSize, 0);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    ASSERT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer), Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
    EXPECT_GT(buffer.mediaAVBuffer->memory_->GetSize(), 0U);

    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(mpeg4Plugin_->ReadSampleZeroCopy(trackId, MediaAVBuffer::CreateAVBuffer(),
        MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleMemoryValidBranch_001
 * @tc.desc: Verify MPEG4SampleMemory valid sample and boundary offset branches
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleMemoryValidBranch_001, TestSize.Level1)
{
    constexpr int32_t sampleSize = 8;
    constexpr int32_t startOffset = 3;
    auto sample = CreateMpeg4Sample(sampleSize, true);
    ASSERT_NE(sample, nullptr);
    ASSERT_NE(sample->data, nullptr);

    Plugins::MPEG4::MPEG4SampleMemory memory(sample, startOffset);
    EXPECT_EQ(memory.GetMemoryType(), MemoryType::CUSTOM_MEMORY);
    EXPECT_EQ(memory.GetAddr(), sample->data.get() + startOffset);
    EXPECT_EQ(memory.GetCapacity(), sampleSize - startOffset);
    EXPECT_EQ(memory.GetSize(), sampleSize - startOffset);
    EXPECT_EQ(memory.GetOffset(), 0);
    EXPECT_EQ(memory.Write(nullptr, sampleSize), 0);

    Plugins::MPEG4::MPEG4SampleMemory endOffsetMemory(sample, sampleSize);
    EXPECT_EQ(endOffsetMemory.GetAddr(), sample->data.get() + sampleSize);
    EXPECT_EQ(endOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(endOffsetMemory.GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleMemoryInvalidBranch_001
 * @tc.desc: Verify MPEG4SampleMemory invalid sample, data, size and offset branches
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleMemoryInvalidBranch_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleMemory nullSampleMemory(nullptr);
    EXPECT_EQ(nullSampleMemory.GetAddr(), nullptr);
    EXPECT_EQ(nullSampleMemory.GetCapacity(), 0);
    EXPECT_EQ(nullSampleMemory.GetSize(), 0);
    EXPECT_EQ(nullSampleMemory.GetOffset(), 0);

    auto sampleWithoutData = CreateMpeg4Sample(8, false);
    Plugins::MPEG4::MPEG4SampleMemory nullDataMemory(sampleWithoutData);
    EXPECT_EQ(nullDataMemory.GetAddr(), nullptr);
    EXPECT_EQ(nullDataMemory.GetCapacity(), 0);
    EXPECT_EQ(nullDataMemory.GetSize(), 0);

    auto negativeSizeSample = CreateMpeg4Sample(-1, false);
    negativeSizeSample->data = std::make_unique<uint8_t[]>(1);
    Plugins::MPEG4::MPEG4SampleMemory negativeSizeMemory(negativeSizeSample);
    EXPECT_EQ(negativeSizeMemory.GetAddr(), nullptr);
    EXPECT_EQ(negativeSizeMemory.GetCapacity(), 0);
    EXPECT_EQ(negativeSizeMemory.GetSize(), 0);

    auto validSample = CreateMpeg4Sample(8, true);
    Plugins::MPEG4::MPEG4SampleMemory negativeOffsetMemory(validSample, -1);
    EXPECT_EQ(negativeOffsetMemory.GetAddr(), nullptr);
    EXPECT_EQ(negativeOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(negativeOffsetMemory.GetSize(), 0);

    Plugins::MPEG4::MPEG4SampleMemory overflowOffsetMemory(validSample, validSample->size + 1);
    EXPECT_EQ(overflowOffsetMemory.GetAddr(), nullptr);
    EXPECT_EQ(overflowOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(overflowOffsetMemory.GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer full-frame read results on h264 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    ASSERT_NO_FATAL_FAILURE(CompareReadDigest(mpeg4Digest, ffmpegDigest));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_WebVtt_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer WebVTT sample attributes on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_MP4_WebVtt_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_WEBVTT);
    InitMpeg4Plugin(TEST_MP4_PATH_WEBVTT);

    auto ffmpegTrackId = FindTrackByMime(ffmpegMediaInfo_, WEBVTT_MIME);
    auto mpeg4TrackId = FindTrackByMime(mpeg4MediaInfo_, WEBVTT_MIME);
    ASSERT_NE(ffmpegTrackId, INVALID_TRACK_ID);
    ASSERT_NE(mpeg4TrackId, INVALID_TRACK_ID);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(ffmpegTrackId), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(mpeg4TrackId), Status::OK);

    auto readSample = [this](const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
        const std::shared_ptr<MediaAVBuffer>& buffer) {
        return ReadSampleWithRetry(plugin, trackId, buffer, ReadMode::COPY);
    };
    auto isEos = [this](const std::shared_ptr<MediaAVBuffer>& buffer) { return IsEos(buffer); };
    auto ffmpegSamples = ReadTrackSamples(ffmpegPlugin_, ffmpegTrackId, readSample, isEos);
    auto mpeg4Samples = ReadTrackSamples(mpeg4Plugin_, mpeg4TrackId, readSample, isEos);
    ASSERT_FALSE(ffmpegSamples.empty());
    CompareWebVttFrameSamples(mpeg4Samples, ffmpegSamples);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_WebVttPendingCue_001
 * @tc.desc: Verify WebVTT pending cues are flushed by track and keep sample duration priority
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_WebVttPendingCue_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    auto track0 = MakeWebVttTrack(0, 10000);
    auto track1 = MakeWebVttTrack(1, 20000);
    track0->next = track1;
    plugin->firstTrack_ = track0;
    plugin->lastTrack_ = track1;
    plugin->mediaInfo_.tracks.resize(2);
    plugin->mediaInfo_.tracks[0].Set<Tag::MIME_TYPE>(WEBVTT_MIME);
    plugin->mediaInfo_.tracks[0].Set<Tag::MEDIA_TIME_SCALE>(1000);
    plugin->mediaInfo_.tracks[1].Set<Tag::MIME_TYPE>(WEBVTT_MIME);
    plugin->mediaInfo_.tracks[1].Set<Tag::MEDIA_TIME_SCALE>(1000);
    plugin->mediaInfo_.general.Set<Tag::MEDIA_DURATION>(60000);
    ASSERT_EQ(plugin->cacheQueue_.AddTrackQueue(0), Status::OK);
    ASSERT_EQ(plugin->cacheQueue_.AddTrackQueue(1), Status::OK);

    plugin->pendingWebVttSamples_[0] = MakePendingWebVttSample(1000, 2000);
    plugin->pendingWebVttSamples_[1] = MakePendingWebVttSample(5000, 1000);
    ASSERT_EQ(plugin->FlushPendingWebVTTSample(0), Status::OK);
    EXPECT_EQ(plugin->pendingWebVttSamples_.count(0), 0U);
    EXPECT_EQ(plugin->pendingWebVttSamples_.count(1), 1U);
    ASSERT_TRUE(plugin->cacheQueue_.HasCache(0));
    ASSERT_FALSE(plugin->cacheQueue_.HasCache(1));
    auto flushedTrack0 = plugin->cacheQueue_.Front(0);
    ASSERT_NE(flushedTrack0, nullptr);
    ASSERT_NE(flushedTrack0->sample, nullptr);
    EXPECT_EQ(flushedTrack0->sample->duration, 2000);

    ASSERT_EQ(plugin->FlushPendingWebVTTSamples(), Status::OK);
    EXPECT_TRUE(plugin->pendingWebVttSamples_.empty());
    ASSERT_TRUE(plugin->cacheQueue_.HasCache(1));
    auto flushedTrack1 = plugin->cacheQueue_.Front(1);
    ASSERT_NE(flushedTrack1, nullptr);
    ASSERT_NE(flushedTrack1->sample, nullptr);
    EXPECT_EQ(flushedTrack1->sample->duration, 1000);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_WebVttContinuousCue_001
 * @tc.desc: Verify continuous WebVTT cues update previous cue duration with next cue pts
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_WebVttContinuousCue_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    auto track = MakeWebVttTrack(0, 10000);
    plugin->firstTrack_ = track;
    plugin->lastTrack_ = track;
    plugin->mediaInfo_.tracks.resize(1);
    plugin->mediaInfo_.tracks[0].Set<Tag::MIME_TYPE>(WEBVTT_MIME);
    ASSERT_EQ(plugin->cacheQueue_.AddTrackQueue(0), Status::OK);

    auto firstCue = MakeWebVttCueSample(1000, 100, "first");
    auto secondCue = MakeWebVttCueSample(2500, 100, "second");
    ASSERT_EQ(plugin->ProcessWebVTTSampleForCache(track, firstCue), Status::OK);
    EXPECT_FALSE(plugin->cacheQueue_.HasCache(0));
    ASSERT_EQ(plugin->ProcessWebVTTSampleForCache(track, secondCue), Status::OK);
    ASSERT_TRUE(plugin->cacheQueue_.HasCache(0));
    auto flushedFirstCue = plugin->cacheQueue_.Front(0);
    ASSERT_NE(flushedFirstCue, nullptr);
    ASSERT_NE(flushedFirstCue->sample, nullptr);
    EXPECT_EQ(flushedFirstCue->sample->pts, 1000);
    EXPECT_EQ(flushedFirstCue->sample->duration, 1500);
    EXPECT_EQ(plugin->pendingWebVttSamples_.count(0), 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_WebVttSeekAndTimeScaleBoundary_001
 * @tc.desc: Verify WebVTT seek fallback index and rounded timescale boundary handling
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_WebVttSeekAndTimeScaleBoundary_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.mimeType_ = MimeType::TEXT_WEBVTT;
    helper.totalDuration_ = 5000;
    helper.sampleIndexEntry_ = {
        {.dts = 0, .pts = 0, .flag = Plugins::MPEG4::Sample::SYNC_FRAME},
        {.dts = 1000, .pts = 1000, .flag = Plugins::MPEG4::Sample::SYNC_FRAME},
    };
    int64_t currentIndex = 1;
    uint32_t sampleIndex = 99;
    EXPECT_TRUE(helper.ResolveTextWebVttSample(1000, currentIndex, sampleIndex));
    EXPECT_EQ(sampleIndex, 1U);

    int64_t result = 0;
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRound(std::numeric_limits<int64_t>::min(), 1, 1, &result),
        Status::OK);
    EXPECT_EQ(result, std::numeric_limits<int64_t>::min());
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRoundToTimeBase(std::numeric_limits<int64_t>::min(), 1, 2, &result),
        Status::OK);
    EXPECT_EQ(result, std::numeric_limits<int64_t>::min() / 2);
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRound(std::numeric_limits<int64_t>::max(), 2, 1, &result),
        Status::ERROR_INVALID_DATA);
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRoundToTimeBase(1, 1, 0, &result), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg edit-list fixed index attributes on h264 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompareMovFixIndexResultWithFfmpeg(TEST_MP4_PATH_H264_B));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_002
 * @tc.desc: Compare MPEG4 and FFmpeg edit-list fixed index attributes on h265 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompareMovFixIndexResultWithFfmpeg(TEST_MP4_PATH_H265_B));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSampleZeroCopy_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer zero-copy first-sample result on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSampleZeroCopy_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegDigest = ReadFirstSample(ffmpegPlugin_, ffmpegMediaInfo_, ReadMode::ZERO_COPY);
    auto mpeg4Digest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::ZERO_COPY);
    ASSERT_NO_FATAL_FAILURE(CompareSampleDigest(mpeg4Digest, ffmpegDigest));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer seek and remaining-frame read results on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_CompareWithFfmpeg_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {0, 1000, 2000, 3000};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(CompareSeekReadResult(TEST_MP4_PATH_H264_B, seekTimes, seekModes));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_SourceFirstSample_001
 * @tc.desc: Verify first video sample after PTS seek against source sync-sample timeline on B-frame mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_SourceFirstSample_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {1920, 2160, 2200, 2440, 2600, 2700};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(CheckSeekFirstVideoSampleWithSourceTimeline(TEST_MP4_PATH_H264_B, seekTimes, seekModes));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_FfprobeFixedFrames_001
 * @tc.desc: Verify PTS seek against fixed ffprobe packet timestamps on B-frame mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_FfprobeFixedFrames_001, TestSize.Level1)
{
    for (const auto& expected : GetH264BFfprobeSeekExpectedResults()) {
        SCOPED_TRACE("seekTime=" + std::to_string(expected.seekTimeMs) +
            ", mode=" + std::to_string(static_cast<int32_t>(expected.seekMode)));
        InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
        auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
        uint32_t videoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, selectedTrackIds);
        ASSERT_NE(videoTrackId, INVALID_TRACK_ID);

        int64_t realSeekTime = 0;
        ASSERT_EQ(mpeg4Plugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, realSeekTime), Status::OK);
        EXPECT_EQ(realSeekTime, expected.realSeekTimeNs);

        AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, videoTrackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
        EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
        EXPECT_EQ(buffer.mediaAVBuffer->pts_, expected.ptsUs);
        EXPECT_EQ(buffer.mediaAVBuffer->dts_, expected.dtsUs);
        EXPECT_EQ(buffer.mediaAVBuffer->duration_, expected.durationUs);
        EXPECT_NE(buffer.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME), 0U);
        ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_ReadDigest_001
 * @tc.desc: Verify MPEG4 remaining video samples after PTS seek against source timeline on B-frame mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_ReadDigest_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {1920, 2160, 2200, 2440, 2600, 2700};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(CheckSeekReadResultWithSourceTimeline(TEST_MP4_PATH_H264_B, seekTimes, seekModes));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_Uri_001
 * @tc.desc: Verify first video sample after PTS seek against source sync-sample timeline on B-frame mp4 uri source
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_Uri_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {1920, 2160, 2200, 2440, 2600, 2700};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(
        CheckSeekFirstVideoSampleWithSourceTimeline(TEST_MP4_PATH_H264_B, seekTimes, seekModes, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToStart_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer SeekToStart results after reading a sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToStart_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

    auto ffmpegPreSeekDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4PreSeekDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4PreSeekDigest, ffmpegPreSeekDigest);

    ASSERT_EQ(ffmpegPlugin_->SeekToStart(), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SeekToStart(), Status::OK);

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks, ReadMode::COPY);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks, ReadMode::COPY);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer async read results on h264 mp4 under weak network
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompareWeakNetworkReadResult(TEST_MP4_PATH_H264_B, 2560656, 3));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_ThreadRetryRecover_001
 * @tc.desc: Verify async read thread keeps running when ReadSampleData returns ERROR_AGAIN
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_ThreadRetryRecover_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CompareWeakNetworkReadResult(TEST_MP4_PATH_H264_B, 2560656, WEAK_NETWORK_THREAD_RETRY_FAIL_COUNT));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_TimeoutThenRecover_001
 * @tc.desc: Async read returns timeout under weak network and the next read can recover
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_TimeoutThenRecover_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, WEAK_NETWORK_FIRST_READ_FAIL_OFFSET,
        WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT, WEAK_NETWORK_FAIL_DELAY_MS);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);

    auto trackId = selectedTrackIds.front();
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, WEAK_NETWORK_SHORT_TIMEOUT_MS),
        Status::ERROR_WAIT_TIMEOUT);

    auto ret = ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected("/data/test/media/h264_double_video_audio.mp4",
        {602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CheckReadResultWithExpected(TEST_MP4_PATH_H264_B, {103, 174}, {5, 174}, {0, 25, 50, 75, 100}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0004
 * @tc.desc: Explicitly verify hevc mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AUX, {16}, {1}, {0}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0005
 * @tc.desc: Explicitly verify multi-track hevc mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0005, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    std::vector<uint32_t> numbers(430);
    std::iota(numbers.begin(), numbers.end(), 0);
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AVC_AAC, {430, 601}, {430, 3}, numbers));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected("/data/test/media/h264_double_video_audio.mp4",
        {602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500}, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CheckReadResultWithExpected(TEST_MP4_PATH_H264_B, {103, 174}, {5, 174}, {0, 25, 50, 75, 100}, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0004
 * @tc.desc: Explicitly verify hevc mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AUX, {16}, {1}, {0}, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0005
 * @tc.desc: Explicitly verify multi-track hevc mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0005, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    std::vector<uint32_t> numbers(430);
    std::iota(numbers.begin(), numbers.end(), 0);
    ASSERT_NO_FATAL_FAILURE(
        CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AVC_AAC, {430, 601}, {430, 3}, numbers, true));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_ExplicitFrames_0001
 * @tc.desc: Explicitly verify remaining frame counts after seek on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckSeekReadResultWithExpected(TEST_MP4_PATH_H264_B, GetExplicitSeekExpectedResults()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_URI_ExplicitFrames_0001
 * @tc.desc: Explicitly verify remaining frame counts after seek on mp4 uri source
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_URI_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CheckSeekReadResultWithExpected(TEST_MP4_PATH_H264_B, GetExplicitSeekExpectedResults(), true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_IsRefParserSupported_0001
 * @tc.desc: Resource types supported by RefParser
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_IsRefParserSupported_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    bool isSupport = mpeg4Plugin_->IsRefParserSupported();
    EXPECT_EQ(isSupport, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_IsRefParserSupported_0002
 * @tc.desc: Resource types supported by RefParser
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_IsRefParserSupported_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS);
    bool isSupport = mpeg4Plugin_->IsRefParserSupported();
    EXPECT_EQ(isSupport, false);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefUpdatePos_0001
 * @tc.desc: ParserRefUpdatePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefUpdatePos_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    Status ret = mpeg4Plugin_->ParserRefUpdatePos(0, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefUpdatePos_0002
 * @tc.desc: ParserRefUpdatePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefUpdatePos_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS);
    Status ret = mpeg4Plugin_->ParserRefUpdatePos(0, true);
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefUpdatePos_0003
 * @tc.desc: ParserRefUpdatePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefUpdatePos_0003, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    Status ret = mpeg4Plugin_->ParserRefUpdatePos(0, true);
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefInfo_0001
 * @tc.desc: ParserRefInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefInfo_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    Status ret = mpeg4Plugin_->ParserRefInfo();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetFrameLayerInfo_0001
 * @tc.desc: GetFrameLayerInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetFrameLayerInfo_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    std::shared_ptr<AVBuffer> videoSample = AVBuffer::CreateAVBuffer();
    videoSample->dts_ = 0;
    FrameLayerInfo frameLayerInfo;
    Status ret = mpeg4Plugin_->GetFrameLayerInfo(videoSample, frameLayerInfo);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetFrameLayerInfo_0002
 * @tc.desc: GetFrameLayerInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetFrameLayerInfo_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    FrameLayerInfo frameLayerInfo;
    Status ret = mpeg4Plugin_->GetFrameLayerInfo(0, frameLayerInfo);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetGopLayerInfo_0001
 * @tc.desc: GetGopLayerInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetGopLayerInfo_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    GopLayerInfo gopLayerInfo;
    Status ret = mpeg4Plugin_->GetGopLayerInfo(0, gopLayerInfo);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetIFramePos_0001
 * @tc.desc: GetIFramePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetIFramePos_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    std::vector<uint32_t> IFramePos;
    mpeg4Plugin_->GetIFramePos(IFramePos);
    EXPECT_GE(IFramePos.size(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetIFramePos_0002
 * @tc.desc: GetIFramePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetIFramePos_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    std::vector<uint32_t> IFramePos;
    mpeg4Plugin_->GetIFramePos(IFramePos);
    EXPECT_EQ(IFramePos.size(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Dts2FrameId_0001
 * @tc.desc: Dts2FrameId
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Dts2FrameId_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    int64_t dts = 0;
    uint32_t frameId = -1;
    mpeg4Plugin_->Dts2FrameId(dts, frameId);
    EXPECT_EQ(frameId, 2);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Dts2FrameId_0002
 * @tc.desc: Dts2FrameId
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Dts2FrameId_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    int64_t dts = -1;
    uint32_t frameId = -1;
    Status ret = mpeg4Plugin_->Dts2FrameId(dts, frameId);
    EXPECT_EQ(ret, Status::ERROR_OUT_OF_RANGE);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_FrameId2SeekMs_0001
 * @tc.desc: FrameId2SeekMs
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_FrameId2SeekMs_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    uint32_t frameId = 0;
    int64_t seekMs = -1;
    mpeg4Plugin_->FrameId2SeekMs(frameId, seekMs);
    EXPECT_EQ(seekMs, 0);
}
} // namespace Media
} // namespace OHOS
