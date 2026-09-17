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

#define MEDIA_PLUGIN
#define HST_LOG_TAG "MPEG4BoxParser"
#include <algorithm>
#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <limits>
#include <regex>
#include <set>
#include <array>
#include "avc_parser_impl.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "demuxer_utils.h"
#include "securec.h"
#include "mpeg4_box_parser.h"
#include "mpeg4_format_checker.h"
#include "mpeg4_utils.h"
#include "meta/video_types.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4BoxParser" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
constexpr uint32_t UINT32_SIZE = 4;
constexpr uint64_t MIN_FLOAT_SIZE = 4;
constexpr int32_t BYTE_TO_BITS = 8;
constexpr int32_t CREATION_TIME_OFFSET = 4;
constexpr int32_t CONVERT_SCALE = 1 << 16;
constexpr uint16_t DISPLAY_MATRIX_ROWS = 3;
constexpr uint16_t DISPLAY_MATRIX_COLS = 3;
constexpr uint32_t DISPLAY_MATRIX_SIZE = 36;
constexpr int64_t DYNAMIC_SIZE_V0 = 24;
constexpr int64_t DYNAMIC_SIZE_V1 = 36;
constexpr int64_t DYNAMIC_MATRIX_OFFSET_V0 = 36;
constexpr int64_t DYNAMIC_MATRIX_OFFSET_V1 = 48;
constexpr int64_t DEFAULT_HEAD_SIZE = 8;
constexpr int64_t ESDS_READ_SIZE = 13;
constexpr int32_t MATRIX_SIZE = 4;
constexpr int32_t MAX_DEPTH = 10;
// Maximum payload size for non-mdat atoms; excludes the atom header.
constexpr uint64_t MAX_NON_MDAT_ATOM_PAYLOAD_SIZE = 64ULL * 1024 * 1024;
constexpr int32_t MDAT_ATOM_TYPE = FourccType("mdat");
constexpr uint64_t MAX_TITLE_SIZE = 24;
constexpr uint64_t MAX_EXTRA_DATA_SIZE = 1024 * 1024;
constexpr uint64_t MAX_DATA_SIZE = 256;
constexpr uint64_t MIN_COLOR_SIZE = 4;
constexpr uint64_t MIN_COLOR_PARAM_SIZE = 6;
constexpr uint64_t MIN_MVHD_SIZE = 32;
constexpr uint64_t SUPPORT_DATA_TYPE = 4;
Status ConvertTimeScaleRound(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result);
Status ConvertTimeScaleRoundToTimeBase(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result);
Status ConvertTrackTimeToUs(int64_t trackTime, int64_t trackTimeScale, int64_t* result);
constexpr uint64_t MIN_TKHD_SIZE = 84;
constexpr uint64_t MIN_HDLR_DATA_SIZE = 24;
constexpr uint64_t MIN_BOX_SIZE = 16;
constexpr uint64_t MIN_TFHD_SIZE = 8;
constexpr uint64_t MIN_ELST_SIZE = 8;
constexpr uint64_t MIN_VIDEO_SAMPLE_ENTRY_SIZE = 78;
constexpr uint64_t MIN_AUDIO_SAMPLE_ENTRY_SIZE = 28;
constexpr uint64_t MIN_ACLR_SIZE = 13;
constexpr uint64_t MIN_SIDX_SIZE = 12;
constexpr uint64_t MAX_SIDX_SIZE = MAX_EXTRA_DATA_SIZE;
constexpr uint32_t SIDX_ENTRY_SIZE = 12;
constexpr uint32_t MAX_SIDX_ENTRY_COUNT = 8192;
constexpr uint64_t MIN_TIMED_METADATA_SAMPLE_ENTRY_SIZE = 16;
constexpr uint64_t MIN_TRADITIONAL_METADATA_SIZE = 4;
constexpr uint64_t MIN_LOCI_SIZE = 20;
constexpr uint64_t MIN_CLLI_SIZE = 4;
constexpr uint64_t MIN_MDCV_SIZE = 24;
constexpr uint64_t MIN_TENC_SIZE = 20;
constexpr uint64_t MIN_PSSH_SIZE = 20;
constexpr uint64_t MIN_SENC_SIZE = 8;
constexpr uint64_t MIN_SAIZ_SIZE = 8;
constexpr uint64_t MIN_SAIO_SIZE = 8;
constexpr uint64_t MIN_SCHM_SIZE = 8;
constexpr uint32_t ATOM_TYPE_OFFSET = 4;
constexpr uint64_t PSSH_VERSION_FLAGS_SIZE = 4;
constexpr uint32_t TENC_PATTERN_BYTE_SHIFT = 4;
constexpr uint32_t TENC_PATTERN_BYTE_MASK = 0x0F;
constexpr uint32_t SENC_FLAGS_SUBSAMPLE_ENCRYPTION = 0x01;
constexpr uint32_t SENC_FLAGS_PIFF_SAMPLE_ENCRYPTION = 0x02;
constexpr uint32_t SAIZ_SAIO_FLAGS_AUX_INFO_TYPE = 0x01;
constexpr uint32_t TREX_TRACK_ID_OFFSET = 4;
constexpr uint32_t DRM_IV_SIZE_8 = 8;
constexpr uint32_t DRM_IV_SIZE_16 = 16;
constexpr uint32_t TENC_V0_IS_PROTECTED_OFFSET = 6;
constexpr uint32_t TENC_V0_PER_SAMPLE_IV_SIZE_OFFSET = 7;
constexpr uint32_t TENC_V0_KID_OFFSET = 8;
constexpr uint32_t TENC_V1_IS_PROTECTED_OFFSET = 5;
constexpr uint32_t TENC_V1_PER_SAMPLE_IV_SIZE_OFFSET = 6;
constexpr uint32_t TENC_V1_KID_OFFSET = 7;
constexpr uint32_t TENC_PATTERN_BYTE_OFFSET = 5;
constexpr uint32_t SUBSAMPLE_COUNT_SIZE = 2;
constexpr uint32_t SUBSAMPLE_CLEAR_HEADER_LEN_SIZE = 2;
constexpr uint32_t SUBSAMPLE_ENTRY_SIZE = 6;
constexpr uint32_t SAIZ_SIZE_COUNT_SIZE = 5;
constexpr uint32_t SAIO_ENTRY_COUNT_SIZE = 4;
constexpr uint32_t AUX_INFO_TYPE_SIZE = 8;
constexpr uint32_t TREX_TRACK_ID_SIZE = 4;
constexpr int64_t ROUND_HALF_DIVISOR = 2;
constexpr uint32_t INT64_SIGN_BIT_SHIFT = 63;
constexpr uint64_t JPEG_SIGNATURE_SIZE = 4;
constexpr uint64_t JPEG_MARKER_PREFIX_SIZE = 2;
constexpr uint64_t JPEG_SEGMENT_LENGTH_SIZE = 2;
constexpr uint64_t JPEG_MIN_SEGMENT_SCAN_SIZE = 9;
constexpr uint16_t JPEG_SOI = 0xD8;
constexpr uint16_t JPEG_EOI = 0xD9;
constexpr uint16_t JPEG_SOF_MIN_PRIMARY = 0xC0;
constexpr uint16_t JPEG_SOF_MAX_PRIMARY = 0xC3;
constexpr uint16_t JPEG_SOF_MIN_SECONDARY = 0xC5;
constexpr uint16_t JPEG_SOF_MAX_SECONDARY = 0xC7;
constexpr uint16_t JPEG_SOF_MIN_TERTIARY = 0xC9;
constexpr uint16_t JPEG_SOF_MAX_TERTIARY = 0xCB;
constexpr uint16_t JPEG_SOF_MIN_QUATERNARY = 0xCD;
constexpr uint16_t JPEG_SOF_MAX_QUATERNARY = 0xCF;
constexpr uint64_t JPEG_SOF_MIN_SEGMENT_SIZE = 7;
constexpr uint64_t JPEG_SOF_HEIGHT_OFFSET = 3;
constexpr uint64_t JPEG_SOF_WIDTH_OFFSET = 5;
constexpr int32_t AUDIO_BITS_U8 = 8;
constexpr int32_t AUDIO_BITS_S16 = 16;
constexpr int32_t AUDIO_BITS_S24 = 24;
constexpr int32_t AUDIO_DEFAULT_SAMPLE_PER_FRAME = 1024;
constexpr int32_t AV3A_STEREO_CHANNELS = 2;
constexpr int32_t AV3A_5POINT1POINT4_CHANNELS = 10;
constexpr int32_t AV3A_7POINT1POINT4_CHANNELS = 12;
constexpr int32_t AV3A_LEGACY_5POINT1POINT4_CHANNELS = 14;
constexpr int32_t AV3A_HOA_ORDER3_CHANNELS = 16;
constexpr int32_t AV3A_STEREO_SAMPLE_RATE = 48000;
constexpr int32_t MPEG4_ROOT_DEPTH = 0;  // Root level atom, ftyp, moov, mdat etc.
constexpr int32_t MPEG4_DEPTH_ONE = 1;  // moov's children atom, mvhd, trak, mvex etc.
constexpr int32_t MPEG4_DEPTH_TWO = 2;  // trak's direct children atom, tkhd, mdia, edts etc.
constexpr int32_t MPEG4_DEPTH_THREE = 3;  // mdia's direct children atom, mdhd, hdlr, minf etc.
constexpr int32_t MPEG4_DEPTH_FOUR = 4;  // minf's direct children atom, vmhd, smhd, stbl etc.
constexpr uint8_t ID3V1_GENRE_MAX = 191;
constexpr uint64_t QUICKTIME_EPOCH_OFFSET = 2082844800;
constexpr size_t OFFSET_V0 = 12;
constexpr size_t OFFSET_V1 = 20;
constexpr size_t DURATION_OFFSET_V0 = 16;
constexpr size_t DURATION_OFFSET_V1 = 24;
constexpr size_t DIRECT_META_PATH_SIZE = 2;  // moov/meta 直接结构的路径大小
constexpr uint8_t AOT_SBR = 5;
constexpr uint8_t AOT_PS = 29;
constexpr uint8_t AAC_ER_LD = 23;
constexpr uint8_t AAC_ER_ELD = 39;
// ESDS 描述符标签常量
constexpr uint8_t MPEG4_ES_DESCR_TAG = 0x03;
constexpr uint8_t MPEG4_DEC_CONFIG_DESCR_TAG = 0x04;
constexpr uint8_t MPEG4_DEC_SPECIFIC_DESCR_TAG = 0x05;
constexpr float LUMI_SCALE = 0.0001;
constexpr float PRIMARY_SCALE = 0.00002;
constexpr uint64_t CREATION_TIME_SCALE = 1000000;
// 辅助轨相关常量
constexpr uint32_t MAX_TRACK_REFERENCE_COUNT = 1000;
constexpr uint32_t MAX_METADATA_KEYS_COUNT = 100000;
// location info
constexpr int32_t PRECISION = 4;
constexpr int32_t ALTITUDE_PRECISION = 6;
constexpr int32_t LATITUDE_WIDTH = 8;
constexpr int32_t LONGITUDE_WIDTH = 9;
constexpr uint32_t FULLBOX_PREFIX_SIZE = 4;

constexpr std::array<size_t, 4> MATRIX_2X2_INDICES = {0, 1, 3, 4}; // [0][0], [0][1], [1][0], [1][1]
constexpr std::array<size_t, 3> MATRIX_DIAGONAL_INDICES = {0, 4, 8}; // [0][0], [1][1], [2][2]
constexpr std::array<size_t, 6> MATRIX_OFF_DIAGONAL_INDICES = {1, 2, 3, 5, 6, 7}; // 非对角线元素

constexpr int32_t PSSH_UUID_SIZE = 16;

constexpr uint8_t VORBIS_EXTRADATA_SIZE = 10;
constexpr uint8_t OFFSET_4 = 4;
constexpr uint8_t OFFSET_5 = 5;
constexpr uint8_t OFFSET_6 = 6;
constexpr uint8_t OFFSET_7 = 7;
constexpr uint8_t OFFSET_8 = 8;
constexpr uint8_t OFFSET_9 = 9;

void SetTrackMediaType(Meta &format, TrackType trackType)
{
    MediaType mediaType = GetMediaType(trackType);
    FALSE_RETURN_MSG(mediaType != MediaType::UNKNOWN,
        "Parse track type failed: " PUBLIC_LOG_D32, static_cast<int32_t>(trackType));
    format.Set<Tag::MEDIA_TYPE>(mediaType);
}

struct PathAdder {
    PathAdder(std::vector<uint32_t> *path, uint32_t atomType): path_(path)
    {
        path_->emplace_back(atomType);
    }
    ~PathAdder()
    {
        path_->pop_back();
    }
private:
    std::vector<uint32_t>* path_;

    PathAdder(const PathAdder&);
    PathAdder& operator=(const PathAdder &);
};

// LCOV_EXCL_START
int64_t GetMaxSamplePtsEnd(const std::shared_ptr<MPEG4SampleHelper>& sampleHelper)
{
    int64_t maxPtsEnd = 0;
    for (uint32_t i = 0; sampleHelper != nullptr && i < sampleHelper->GetSampleCount(); ++i) {
        MPEG4SampleHelper::SampleIndexView sample;
        Status ret = sampleHelper->GetSampleIndexView(i, sample);
        if (ret != Status::OK) {
            MEDIA_LOG_E("Get sample index view failed when calculating max pts end, index=" PUBLIC_LOG_U32
                ", count=" PUBLIC_LOG_ZU ", ret=" PUBLIC_LOG_D32,
                i, sampleHelper->GetSampleCount(), static_cast<int32_t>(ret));
            return 0;
        }
        if (sample.duration <= 0 || sample.pts < 0) {
            continue;
        }
        int64_t ptsEnd = 0;
        if (__builtin_add_overflow(sample.pts, static_cast<int64_t>(sample.duration), &ptsEnd)) {
            continue;
        }
        maxPtsEnd = std::max(maxPtsEnd, ptsEnd);
    }
    return maxPtsEnd;
}

int64_t GetMaxSampleDtsEnd(const std::shared_ptr<MPEG4SampleHelper>& sampleHelper)
{
    int64_t maxDtsEnd = 0;
    for (uint32_t i = 0; sampleHelper != nullptr && i < sampleHelper->GetSampleCount(); ++i) {
        MPEG4SampleHelper::SampleIndexView sample;
        Status ret = sampleHelper->GetSampleIndexView(i, sample);
        if (ret != Status::OK) {
            MEDIA_LOG_E("Get sample index view failed when calculating max dts end, index=" PUBLIC_LOG_U32
                ", count=" PUBLIC_LOG_ZU ", ret=" PUBLIC_LOG_D32,
                i, sampleHelper->GetSampleCount(), static_cast<int32_t>(ret));
            return 0;
        }
        if (sample.duration <= 0 || sample.dts < 0) {
            continue;
        }
        int64_t dtsEnd = 0;
        if (__builtin_add_overflow(sample.dts, static_cast<int64_t>(sample.duration), &dtsEnd)) {
            continue;
        }
        maxDtsEnd = std::max(maxDtsEnd, dtsEnd);
    }
    return maxDtsEnd;
}
// LCOV_EXCL_STOP

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

static const AudioChannelLayout AAC_CHANNEL_LAYOUT_TABLE[14] = {
    AudioChannelLayout::UNKNOWN,
    AudioChannelLayout::MONO,
    AudioChannelLayout::STEREO,
    AudioChannelLayout::SURROUND,
    AudioChannelLayout::CH_4POINT0,
    AudioChannelLayout::CH_5POINT0_BACK,
    AudioChannelLayout::CH_5POINT1_BACK,
    AudioChannelLayout::CH_7POINT1,
    AudioChannelLayout::UNKNOWN,
    AudioChannelLayout::UNKNOWN,
    AudioChannelLayout::UNKNOWN,
    AudioChannelLayout::CH_6POINT1_BACK,
    AudioChannelLayout::CH_7POINT1,
    AudioChannelLayout::CH_22POINT2
};

static const uint32_t AAC_SAMPLE_RATE_TABLE[] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

constexpr uint64_t AV3A_DCA3_BOX_MIN_SIZE = 5;
constexpr uint64_t AV3A_DCA3_BOX_MAX_SIZE = 64;
constexpr int32_t AV3A_AUDIO_FRAME_SIZE = 1024;
constexpr uint8_t AV3A_LOSSY_CODEC_ID = 2;
constexpr uint8_t AV3A_BASELINE_NN_TYPE = 0;
constexpr uint8_t AV3A_LC_NN_TYPE = 2;
constexpr uint8_t AV3A_CHANNEL_BASED_TYPE = 0;
constexpr uint8_t AV3A_OBJECT_BASED_TYPE = 1;
constexpr uint8_t AV3A_CHANNEL_OBJECT_TYPE = 2;
constexpr uint8_t AV3A_AMBISONIC_TYPE = 3;
constexpr uint8_t AV3A_AMBISONIC_FIRST_ORDER = 1;
constexpr uint8_t AV3A_AMBISONIC_THIRD_ORDER = 3;
constexpr uint8_t AV3A_CHANNEL_CONFIG_MONO = 0;
constexpr uint8_t AV3A_CHANNEL_CONFIG_STEREO = 1;
constexpr uint8_t AV3A_CHANNEL_CONFIG_7_1_4 = 10;

struct Av3aResolutionInfo {
    AudioSampleFormat sampleFormat;
    int32_t bitsPerSample;
};

struct Av3aChannelConfig {
    uint8_t index;
    AudioChannelLayout layout;
    int32_t channels;
};

struct MultiplyDisplayMatrixDataLen {
    size_t lenA = 0;
    size_t lenB = 0;
    size_t lenC = 0;
};

constexpr std::array<Av3aResolutionInfo, 4> AV3A_RESOLUTION_TABLE = {{
    {AudioSampleFormat::SAMPLE_U8, 8},
    {AudioSampleFormat::SAMPLE_S16LE, 16},
    {AudioSampleFormat::SAMPLE_S24LE, 24},
    {AudioSampleFormat::INVALID_WIDTH, 0},
}};

constexpr std::array<Av3aChannelConfig, 11> AV3A_CHANNEL_CONFIG_TABLE = {{
    {0, AudioChannelLayout::MONO, 1},
    {1, AudioChannelLayout::STEREO, 2},
    {2, AudioChannelLayout::CH_4POINT0, 4},
    {3, AudioChannelLayout::CH_5POINT0, 5},
    {4, AudioChannelLayout::CH_5POINT1, 6},
    {5, AudioChannelLayout::CH_7POINT0, 7},
    {6, AudioChannelLayout::CH_7POINT1, 8},
    {7, AudioChannelLayout::CH_5POINT1POINT2, 8},
    {8, AudioChannelLayout::CH_5POINT1POINT4, 10},
    {9, AudioChannelLayout::CH_7POINT1POINT2, 10},
    {10, AudioChannelLayout::CH_7POINT1POINT4, 12},
}};

uint32_t ReadBitsWide(DemuxerBitReader& bitReader, uint8_t numBits)
{
    uint32_t value = 0;
    while (numBits > 0) {
        uint8_t bitsToRead = std::min<uint8_t>(numBits, 8);
        value = (value << bitsToRead) | bitReader.ReadBits(bitsToRead);
        numBits -= bitsToRead;
    }
    return value;
}

uint32_t ReadBitsWidePadded(DemuxerBitReader& bitReader, uint8_t numBits)
{
    if (bitReader.HasBits(numBits)) {
        return ReadBitsWide(bitReader, numBits);
    }
    uint32_t value = 0;
    while (numBits > 0) {
        value <<= 1;
        if (bitReader.HasBits(1)) {
            value |= bitReader.ReadBits(1);
        }
        --numBits;
    }
    return value;
}

uint32_t ShowBitsWide(DemuxerBitReader bitReader, uint8_t numBits)
{
    return ReadBitsWidePadded(bitReader, numBits);
}

void ReadAACSampleRate(DemuxerBitReader& bitReader, uint8_t& samplingFreqIndex, uint32_t& sampleRate)
{
    constexpr uint8_t samplingFreqIndexSize = 4;
    constexpr uint8_t explicitSampleRateSize = 24;
    constexpr uint8_t explicitSampleRateIndex = 0x0F;
    samplingFreqIndex = static_cast<uint8_t>(ReadBitsWidePadded(bitReader, samplingFreqIndexSize));
    if (samplingFreqIndex == explicitSampleRateIndex) {
        sampleRate = ReadBitsWidePadded(bitReader, explicitSampleRateSize);
        return;
    }
    sampleRate = samplingFreqIndex < sizeof(AAC_SAMPLE_RATE_TABLE) / sizeof(AAC_SAMPLE_RATE_TABLE[0]) ?
        AAC_SAMPLE_RATE_TABLE[samplingFreqIndex] : 0;
}

void ReadAACObjectType(DemuxerBitReader& bitReader, uint8_t& audioObjectType)
{
    constexpr uint8_t audioObjectTypeSize = 5;
    constexpr uint8_t audioObjectTypeEscape = 0x1F;
    constexpr uint8_t audioObjectTypeExtensionSize = 6;
    audioObjectType = static_cast<uint8_t>(ReadBitsWidePadded(bitReader, audioObjectTypeSize));
    if (audioObjectType != audioObjectTypeEscape) {
        return;
    }
    audioObjectType = static_cast<uint8_t>(0x20 + ReadBitsWidePadded(bitReader, audioObjectTypeExtensionSize));
}

bool GetAv3aChannelConfig(uint8_t channelNumberIndex, AudioChannelLayout& layout, int32_t& channels)
{
    auto ite = std::find_if(AV3A_CHANNEL_CONFIG_TABLE.begin(), AV3A_CHANNEL_CONFIG_TABLE.end(),
        [channelNumberIndex](const auto& item) {
            return item.index == channelNumberIndex;
        });
    if (ite == AV3A_CHANNEL_CONFIG_TABLE.end()) {
        return false;
    }
    layout = ite->layout;
    channels = ite->channels;
    return true;
}

MPEG4AtomParser::MPEG4AtomParser()
    : firstTrack_(nullptr),
      lastTrack_(nullptr),
      isIsom_(false),
      hasMdatBox_(false),
      hasSidxBox_(false),
      isDistributedSidx_(false),
      hasIncompleteSidxReference_(false),
      hasMoofBox_(false),
      moovFound_(false),
      seekable_(Seekable::SEEKABLE),
      userFormat_(std::make_shared<Meta>())
{
    MEDIA_LOG_D("In");
    InitParseTable();
    MEDIA_LOG_D("Out");
}

MPEG4AtomParser::~MPEG4AtomParser()
{
    MEDIA_LOG_D("In");
    firstTrack_.reset();
    lastTrack_.reset();
    dataSource_.reset();
    dataReader_.reset();
    MPEG4ParseTable_.clear();
    MEDIA_LOG_D("Out");
}

inline bool MPEG4AtomParser::IsValidTrackIndex(const MediaInfo& mediaInfo, uint32_t index)
{
    return index < mediaInfo.tracks.size();
}

inline bool MPEG4AtomParser::IsLastTrackValid()
{
    return lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr &&
        IsValidTrackIndex(mediaInfo_, lastTrack_->trackIndex);
}

bool CheckBoxDataAllocSize(uint64_t dataSize, const std::string& boxType)
{
    if (dataSize <= MPEG4_MAX_BOX_DATA_SIZE) {
        return true;
    }
    MEDIA_LOG_E("Invalid " PUBLIC_LOG_S " data size: " PUBLIC_LOG_U64, boxType.c_str(), dataSize);
    return false;
}

bool IsUnsupportedFmp4Atom(uint32_t atomType)
{
    switch (atomType) {
        case FourccType("moof"):
        case FourccType("mvex"):
        case FourccType("sidx"):
        case FourccType("mfra"):
        case FourccType("mfro"):
        case FourccType("tfra"):
        case FourccType("mfhd"):
        case FourccType("traf"):
        case FourccType("tfhd"):
        case FourccType("tfdt"):
        case FourccType("trun"):
        case FourccType("trex"):
            return true;
        default:
            return false;
    }
}

inline int32_t ConvFp(int32_t x)
{
    return static_cast<int32_t>(x / (CONVERT_SCALE));
}

inline double ConvDp(int32_t x)
{
    return static_cast<double>(x) / (CONVERT_SCALE);
}

std::array<int32_t, MATRIX_SIZE> Extract2x2Transform(const int32_t* displayMatrix)
{
    std::array<int32_t, MATRIX_SIZE> result;
    for (size_t i = 0; i < MATRIX_2X2_INDICES.size(); ++i) {
        result[i] = ConvFp(displayMatrix[MATRIX_2X2_INDICES[i]]);
    }
    return result;
}

bool IsIdentityMatrix(const int32_t* matrix)
{
    // 检查对角线元素
    if (matrix[MATRIX_DIAGONAL_INDICES[0]] != CONVERT_SCALE ||  // [0][0]
        matrix[MATRIX_DIAGONAL_INDICES[1]] != CONVERT_SCALE ||  // [1][1]
        matrix[MATRIX_DIAGONAL_INDICES[2]] != (1 << 0x1E)) {    // [2][2]
        return false;
    }

    // 检查非对角线元素是否都为0
    for (size_t idx : MATRIX_OFF_DIAGONAL_INDICES) {
        FALSE_RETURN_V_NOLOG(matrix[idx] == 0, false);
    }
    return true;
}

bool IsDisplayMatrixIdentity(const int32_t matrix[3][3], const size_t len)
{
    FALSE_RETURN_V_NOLOG(len == 36UL, false);
    return IsIdentityMatrix(reinterpret_cast<const int32_t*>(matrix));
}

void MultiplyDisplayMatrix(const int32_t a[3][3], const int32_t b[3][3], int32_t result[3][3],
                           MultiplyDisplayMatrixDataLen len)
{
    FALSE_RETURN_MSG(a != nullptr && b != nullptr && result != nullptr, "Invalid params");
    FALSE_RETURN_MSG(len.lenA == 36UL && len.lenB == 36UL && len.lenC == 36UL, "Invalid params len");
    // MPEG4 display matrix的位移量，用于定点数运算
    // [0][0], [1][1] 元素使用16位小数
    // [2][2] 元素使用30位小数
    const int32_t shift[3] = { 16, 16, 30 };
    const int32_t elementCount = 9; // 3x3矩阵有9个元素
    // 初始化结果矩阵
    std::fill(&result[0][0], &result[0][0] + elementCount, 0);
    // 执行矩阵乘法：result = a * b
    for (uint16_t i = 0; i < DISPLAY_MATRIX_ROWS; ++i) {
        for (uint16_t j = 0; j < DISPLAY_MATRIX_COLS; ++j) {
            for (uint16_t k = 0; k < DISPLAY_MATRIX_COLS; ++k) {
                // 使用64位避免中间计算溢出，然后右移恢复定点数精度
                result[i][j] += static_cast<int32_t>(
                    (static_cast<int64_t>(a[i][k]) * b[k][j]) >> shift[k]);
            }
        }
    }
}

void PrintMatrixToLog(const int32_t* matrix, const std::string& matrixName)
{
    MEDIA_LOG_D(PUBLIC_LOG_S ": [" PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " "
        PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 " " PUBLIC_LOG_D32 "]",
        matrixName.c_str(), matrix[0], matrix[1], matrix[2], matrix[3], matrix[4],
        matrix[5], matrix[6], matrix[7], matrix[8]);
}

// LCOV_EXCL_START
static int64_t ReadChannelSet(uint32_t tag)
{
    const uint32_t chanTopBackRightTag = 18;
    const uint32_t chanWideLeftTag = 35;
    const uint32_t chanWideRightTag = 36;
    const uint32_t chanLowFrequency2 = 37;
    const uint32_t chanStereoLeft = 38;
    const uint32_t chanStereoRight = 39;

    FALSE_RETURN_V_NOLOG(tag <= 0 || tag > chanTopBackRightTag, 1U << (tag - 1));

    switch (tag) {
        case chanWideLeftTag:
            return static_cast<int64_t>(AudioChannelSet::WIDE_LEFT);
        case chanWideRightTag:
            return static_cast<int64_t>(AudioChannelSet::WIDE_RIGHT);
        case chanLowFrequency2:
            return static_cast<int64_t>(AudioChannelSet::LOW_FREQUENCY_2);
        case chanStereoLeft:
            return static_cast<int64_t>(AudioChannelSet::STEREO_LEFT);
        case chanStereoRight:
            return static_cast<int64_t>(AudioChannelSet::STEREO_RIGHT);
        default:
            return 0;
    }
}

int64_t MPEG4AtomParser::GetChannelLayoutByBitmap(uint32_t tag, uint32_t bitmap)
{
    if (tag == (1 << 0x10)) {
        return bitmap < 0x040000 ? bitmap : 0;
    }
    return 0;
}
// LCOV_EXCL_STOP

Status MPEG4AtomParser::ParseDisplayMatrix(const uint8_t* data, int32_t matrix[3][3])
{
    FALSE_RETURN_V_MSG_E(data != nullptr && matrix != nullptr, Status::ERROR_INVALID_PARAMETER, "Invalid params");
    // MPEG4 display matrix按行存储：[a b u c d v x y w]
    // 矩阵格式：[a b u]
    //          [c d v]
    //          [x y w]
    for (uint16_t i = 0; i < DISPLAY_MATRIX_ROWS; ++i) {
        for (uint16_t j = 0; j < DISPLAY_MATRIX_COLS; ++j) {
            matrix[i][j] = static_cast<int32_t>(GetU32Value(&data[(i * DISPLAY_MATRIX_ROWS + j) * sizeof(int32_t)]));
        }
    }
    PrintMatrixToLog(&matrix[0][0], "displayMatrix");
    return Status::OK;
}

std::string ConvertArrayToString(const int32_t* array, size_t size)
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

VideoOrientationType GetMatrixType(const std::string& value)
{
    auto it = matrixTypes.find(value);
    return (it != matrixTypes.end()) ? it->second : VideoOrientationType::ROTATE_NONE;
}

void MPEG4AtomParser::ParseOrientationFromMatrix(std::shared_ptr<Track> track)
{
    VideoOrientationType orientationType = VideoOrientationType::ROTATE_NONE;
    if (track && track->hasDisplayMatrix && track->displayMatrix) {
        PrintMatrixToLog(track->displayMatrix.get(), "displayMatrix");
        // 转换矩阵：提取前2x2部分并转换为整数
        auto convertedMatrix = Extract2x2Transform(track->displayMatrix.get());
        // 转换为字符串并查找方向类型
        std::string matrixStr = ConvertArrayToString(convertedMatrix.data(), convertedMatrix.size());
        orientationType = GetMatrixType(matrixStr);
    } else {
        MEDIA_LOG_W("Parse orientation info from display matrix failed, set orientation as default 0");
    }

    if (!IsValidTrackIndex(mediaInfo_, track->trackIndex)) {
        MEDIA_LOG_E("Invalid track index: " PUBLIC_LOG_U32, track->trackIndex);
        return;
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_ORIENTATION_TYPE>(orientationType);
}

double CalculateDisplayRotation(const int32_t matrix[9])
{
    FALSE_RETURN_V_MSG_E(matrix != nullptr, 0.0, "Matrix is null");
    constexpr double pi = 3.14159265358979323846;
    constexpr double halfCircle = 180.0;
    double scale[2];
    scale[0] = std::hypot(ConvDp(matrix[0]), ConvDp(matrix[0x03]));
    scale[1] = std::hypot(ConvDp(matrix[1]), ConvDp(matrix[0x04]));

    FALSE_RETURN_V_MSG_W(scale[0] != 0.0 && scale[1] != 0.0, std::numeric_limits<double>::quiet_NaN(),
        "Invalid matrix: zero scale factor");

    double rotationRad = std::atan2(ConvDp(matrix[1]) / scale[1], ConvDp(matrix[0]) / scale[0]);
    double rotation = -rotationRad * halfCircle / pi;
    return rotation;
}

void MPEG4AtomParser::ParseRotationTypeFromMatrix(std::shared_ptr<Track> track)
{
    VideoRotation rotationType = VIDEO_ROTATION_0;
    constexpr int32_t VIDEO_ROTATION_360 = 360;
    if (track && track->hasDisplayMatrix && track->displayMatrix) {
        float rotation = -round(CalculateDisplayRotation(track->displayMatrix.get()));
        if (isnan(rotation)) {
            rotationType = VIDEO_ROTATION_0;
        } else if (rotation < 0) {
            rotation += VIDEO_ROTATION_360;
        }
        switch (static_cast<int32_t>(rotation)) {
            case VIDEO_ROTATION_90:
                rotationType = VIDEO_ROTATION_90;
                break;
            case VIDEO_ROTATION_180:
                rotationType = VIDEO_ROTATION_180;
                break;
            case VIDEO_ROTATION_270:
                rotationType = VIDEO_ROTATION_270;
                break;
            default:
                rotationType = VIDEO_ROTATION_0;
                break;
        }
    } else {
        MEDIA_LOG_W("Parse rotate info from display matrix failed, set rotation as default 0");
        rotationType = VIDEO_ROTATION_0;
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_ROTATION>(rotationType);
}

Status MPEG4AtomParser::ParseContainerAtom(const MPEG4Atom& containerAtom, int32_t depth, ParseContext* ctx)
{
    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->dataOffset, containerAtom.dataSize, &endOffset)) {
        MEDIA_LOG_E("Container atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    // 设置解析起始位置为容器的数据开始位置
    ctx->offset = ctx->dataOffset;
    while (ctx->offset < endOffset) {
        bool isPadding = false;
        Status ret = PrepareChildAtomInContainer(endOffset, ctx, isPadding);
        if (ret != Status::OK) {
            return ret;
        }
        if (isPadding) {
            break;
        }
        ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse atom at offset " PUBLIC_LOG_D64, ctx->offset);
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA,
        "Finished parsing container atom at offset " PUBLIC_LOG_D64 ", expected end at " PUBLIC_LOG_D64,
        ctx->offset, endOffset);

    return Status::OK;
}

Status MPEG4AtomParser::PrepareChildAtomInContainer(int64_t endOffset, ParseContext* ctx, bool& isPadding)
{
    isPadding = false;
    if (ctx->offset + static_cast<int64_t>(sizeof(uint32_t)) > endOffset) {
        MEDIA_LOG_D("Trailing padding at offset " PUBLIC_LOG_D64, ctx->offset);
        ctx->offset = endOffset;
        isPadding = true;
        return Status::OK;
    }
    uint8_t atomSizeData[sizeof(uint32_t)] = {0};
    Status sizeRet = dataReader_->ReadUintData(ctx->offset, atomSizeData, sizeof(atomSizeData));
    FALSE_RETURN_V_MSG_E(sizeRet == Status::OK, sizeRet, "Read child atom size failed");
    uint32_t childSize32 = GetU32Value(atomSizeData);
    if (childSize32 == 0) {
        MEDIA_LOG_D("Zero-size child atom at offset " PUBLIC_LOG_D64 ", treat remaining container as padding",
            ctx->offset);
        ctx->offset = endOffset;
        isPadding = true;
        return Status::OK;
    }
    constexpr int64_t basicHeaderSize = 8;
    constexpr uint64_t basicHeaderSizeU64 = 8;
    int64_t childHeaderEndOffset = 0;
    if (__builtin_add_overflow(ctx->offset, basicHeaderSize, &childHeaderEndOffset) ||
        childHeaderEndOffset > endOffset) {
        MEDIA_LOG_E("Child atom header exceeds container at offset " PUBLIC_LOG_D64, ctx->offset);
        return Status::ERROR_INVALID_DATA;
    }
    uint64_t childSize = childSize32;
    if (childSize32 == 1) {
        Status ret = ReadExtendedChildAtomSize(childHeaderEndOffset, endOffset, ctx, childSize);
        if (ret != Status::OK) {
            return ret;
        }
    }
    uint64_t minChildSize = childSize32 == 1 ? sizeof(uint64_t) + sizeof(uint64_t) : basicHeaderSizeU64;
    FALSE_RETURN_V_MSG_E(childSize >= minChildSize, Status::ERROR_INVALID_DATA,
        "Invalid child atom size: " PUBLIC_LOG_U64, childSize);
    int64_t childEndOffset = 0;
    if (__builtin_add_overflow(ctx->offset, childSize, &childEndOffset) || childEndOffset > endOffset) {
        MEDIA_LOG_E("Child atom exceeds container: end " PUBLIC_LOG_D64 ", container end " PUBLIC_LOG_D64,
            childEndOffset, endOffset);
        return Status::ERROR_INVALID_DATA;
    }
    return Status::OK;
}

Status MPEG4AtomParser::ReadExtendedChildAtomSize(int64_t childHeaderEndOffset, int64_t endOffset, ParseContext* ctx,
    uint64_t& childSize)
{
    int64_t extendedHeaderEndOffset = 0;
    if (__builtin_add_overflow(childHeaderEndOffset, static_cast<int64_t>(sizeof(uint64_t)),
        &extendedHeaderEndOffset) || extendedHeaderEndOffset > endOffset) {
        MEDIA_LOG_E("Child atom extended size exceeds container at offset " PUBLIC_LOG_D64, ctx->offset);
        return Status::ERROR_INVALID_DATA;
    }
    uint8_t extendedSizeData[sizeof(uint64_t)] = {0};
    Status extSizeRet = dataReader_->ReadUintData(childHeaderEndOffset, extendedSizeData,
        sizeof(extendedSizeData));
    FALSE_RETURN_V_MSG_E(extSizeRet == Status::OK, extSizeRet, "Read child atom extended size failed");
    childSize = GetU64Value(extendedSizeData);
    return Status::OK;
}

Status MPEG4AtomParser::ParseFtyp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE && currentAtom.dataSize <= MAX_DATA_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid atom size for ftyp atom");
    FALSE_RETURN_V_MSG_E(depth == MPEG4_ROOT_DEPTH, Status::ERROR_INVALID_PARAMETER, "Invalid ftyp depth");

    auto ftypInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, ftypInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read ftyp atom header failed");
    std::string majorBrand = GetCharValue(&ftypInfo[0]);

    FALSE_RETURN_V_MSG_E(!IsHeifFormat(majorBrand.c_str()), Status::ERROR_UNKNOWN, "HEIF format is not supported");
    FALSE_RETURN_V_MSG_W(IsSupportedBrand(majorBrand),
        Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, "Unsupported file: brand[%{public}s]", majorBrand.c_str());
    uint32_t compatibleBrandsCount = (currentAtom.dataSize - 8) / 4;
    FALSE_RETURN_V_MSG_E(compatibleBrandsCount > 0, Status::ERROR_INVALID_DATA, "Invalid compatible brands number");
    // 一次性读取所有compatible brands数据
    bool hasGltfBrand = false;
    if (compatibleBrandsCount > 0) {
        for (uint32_t i = 0; i < compatibleBrandsCount; ++i) {
            uint32_t pos = 8 + 4 * i;
            FALSE_RETURN_V_MSG_E(pos <= currentAtom.dataSize - sizeof(uint32_t), Status::ERROR_INVALID_DATA,
                                 "Invalid ftypInfo pos number");
            // 解析和存储所有brands, 跳过 minor version，从第8字节开始
            uint32_t compatibleBrand = GetU32Value(&ftypInfo[pos]);
            std::string brandStr = FourccToString(compatibleBrand);
            ctx->compatibleBrands.insert(compatibleBrand);
            hasGltfBrand = hasGltfBrand || compatibleBrand == static_cast<uint32_t>(FourccType("glti"));
        }
    }
    if (hasGltfBrand) {
        mediaInfo_.general.Set<Tag::IS_GLTF>(true);
    }
    mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::MP4);
    if (StartWith(majorBrand.c_str(), "m4a") || StartWith(majorBrand.c_str(), "M4A")) {
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::M4A);
    } else {
        isIsom_ = true; // 若为MPEG4格式，设置isom标志
    }
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    return Status::OK;
}

Status MPEG4AtomParser::ParseMoov(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_ROOT_DEPTH, Status::ERROR_INVALID_PARAMETER, "Invalid moov depth");
    FALSE_RETURN_V_MSG_E(!moovFound_, Status::ERROR_INVALID_DATA, "Duplicate moov atom");

    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse moov atom failed");

    moovFound_ = true;
    return ret;
}

Status MPEG4AtomParser::ParseMdat(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    (void)depth;
    uint64_t effectiveSize = currentAtom.effectiveSize > 0 ? currentAtom.effectiveSize : currentAtom.size;
    FALSE_RETURN_V_NOLOG(effectiveSize > 0, Status::OK);
    FALSE_RETURN_V_MSG_E(ctx != nullptr && ctx->offset >= 0 && effectiveSize <= static_cast<uint64_t>(INT64_MAX) &&
        static_cast<uint64_t>(ctx->offset) <= static_cast<uint64_t>(INT64_MAX) - effectiveSize,
        Status::ERROR_INVALID_DATA, "Invalid mdat effective size");
    if (currentAtom.dataSize > 0) {
        hasMdatBox_ = true;
    }
    ctx->offset += static_cast<int64_t>(effectiveSize);
    return Status::OK;
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseWide(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    uint64_t effectiveSize = currentAtom.effectiveSize > 0 ? currentAtom.effectiveSize : currentAtom.size;
    FALSE_RETURN_V_MSG_E(effectiveSize <= static_cast<uint64_t>(INT64_MAX) && ctx->offset >= 0 &&
        static_cast<uint64_t>(ctx->offset) <= static_cast<uint64_t>(INT64_MAX) - effectiveSize,
        Status::ERROR_INVALID_DATA, "Invalid wide atom effective size");
    if (currentAtom.dataSize < DEFAULT_HEAD_SIZE) {
        ctx->offset += static_cast<int64_t>(effectiveSize);
        return Status::OK;  // continue
    }

    uint8_t headBuffer[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headBuffer, sizeof(headBuffer));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read embedded size failed");

    uint32_t embeddedSize = GetU32Value(headBuffer, sizeof(headBuffer));
    // 如果不是0 sized mdat atom，跳过剩余数据
    if (embeddedSize != 0) {
        ctx->offset += static_cast<int64_t>(effectiveSize);
        return Status::OK;
    }

    uint32_t embeddedType = GetU32Value(&headBuffer[4]);
    // 如果不是mdat类型，跳过剩余数据
    if (embeddedType != static_cast<uint32_t>(FourccType("mdat"))) {
        ctx->offset += static_cast<int64_t>(effectiveSize);
        return Status::OK;
    }

    MPEG4Atom mdatAtom;
    mdatAtom.type = static_cast<int32_t>(embeddedType);
    mdatAtom.size = currentAtom.dataSize - sizeof(headBuffer);
    mdatAtom.effectiveSize = mdatAtom.size;
    mdatAtom.dataSize = mdatAtom.size;
    // 更新ctx指向mdat数据的开始位置
    int64_t originalOffset = ctx->offset;
    int64_t originalDataOffset = ctx->dataOffset;
    ctx->dataOffset = originalDataOffset + sizeof(headBuffer);

    ret = ParseMdat(mdatAtom, depth, ctx);
    ctx->dataOffset = originalDataOffset;
    ctx->offset = originalOffset + static_cast<int64_t>(effectiveSize);
    return ret;
}
// LCOV_EXCL_STOP

std::string FormatTimestamp(uint64_t creationTime)
{
    time_t seconds = static_cast<time_t>(creationTime / CREATION_TIME_SCALE);
    struct tm* timeinfo = gmtime(&seconds);
    FALSE_RETURN_V_NOLOG(timeinfo != nullptr, "");
    char buffer[64];
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo) == 0) {
        MEDIA_LOG_W("Failed to format timestamp");
        return "";
    }
    int len = strlen(buffer);
    int ret = sprintf_s(
        buffer + len, sizeof(buffer) - len, ".%06dZ", static_cast<int>(creationTime % CREATION_TIME_SCALE));
    if (ret == 0) {
        MEDIA_LOG_W("Failed to format timestamp");
    }
    return std::string(buffer);
}

MPEG4AtomParser::MvhdInfo MPEG4AtomParser::ParseMvhdInfo(const uint8_t* headerInfo, const size_t len) const
{
    MvhdInfo mvhdInfo;
    if (len <= 0) {
        MEDIA_LOG_E("Invalid headerInfo");
    }
    uint8_t version = headerInfo[0];
    mvhdInfo.matrixOffset = version == 1 ? DYNAMIC_MATRIX_OFFSET_V1 : DYNAMIC_MATRIX_OFFSET_V0;
    if (version == 1) {
        mvhdInfo.creationTime = GetU64Value(&headerInfo[CREATION_TIME_OFFSET]);
        mvhdInfo.timeScale = GetU32Value(&headerInfo[OFFSET_V1]);
        mvhdInfo.duration = GetU64Value(&headerInfo[DURATION_OFFSET_V1]);
    } else {
        mvhdInfo.creationTime = static_cast<uint64_t>(GetU32Value(&headerInfo[CREATION_TIME_OFFSET]));
        if (mvhdInfo.creationTime > 0 && mvhdInfo.creationTime < QUICKTIME_EPOCH_OFFSET) {
            mvhdInfo.creationTime += QUICKTIME_EPOCH_OFFSET; // Convert to Unix epoch time
        }
        mvhdInfo.timeScale = GetU32Value(&headerInfo[OFFSET_V0]);
        mvhdInfo.duration = static_cast<uint64_t>(GetU32Value(&headerInfo[DURATION_OFFSET_V0]));
    }
    if (mvhdInfo.timeScale <= 0) {
        mvhdInfo.timeScale = 1; // set to default value
    }
    return mvhdInfo;
}

void MPEG4AtomParser::SetMvhdMediaInfo(const MvhdInfo& mvhdInfo)
{
    movieTimeScale_ = static_cast<int32_t>(mvhdInfo.timeScale);
    std::string creationTimeStr;
    if (mvhdInfo.creationTime) {
        uint64_t creationTime = mvhdInfo.creationTime - QUICKTIME_EPOCH_OFFSET;
        creationTimeStr = FormatTimestamp(creationTime * CREATION_TIME_SCALE);
        mediaInfo_.general.Set<Tag::MEDIA_CREATION_TIME>(creationTimeStr);
    }

    int64_t durationUs = 0;
    if (ConvertTimeScaleRound(mvhdInfo.duration, S_TO_US, movieTimeScale_, &durationUs) != Status::OK) {
        MEDIA_LOG_E("Mul overflow");
    }
    if (durationUs > 0) {
        mediaInfo_.general.Set<Tag::MEDIA_DURATION>(durationUs);
    }
}

Status MPEG4AtomParser::ParseMvhdDisplayMatrix(const MPEG4Atom& currentAtom, const uint8_t* headerInfo,
    ParseContext* ctx, const size_t len)
{
    // 解析movie level display matrix
    int32_t matrixOffset = ParseMvhdInfo(headerInfo, len).matrixOffset;
    if (currentAtom.dataSize >= static_cast<uint64_t>(matrixOffset + DISPLAY_MATRIX_SIZE)) {
        Status ret = ParseDisplayMatrix(&headerInfo[matrixOffset], ctx->movieDisplayMatrix);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse movie display matrix");
        ctx->hasMovieDisplayMatrix = !IsDisplayMatrixIdentity(ctx->movieDisplayMatrix,
                                                              sizeof(ctx->movieDisplayMatrix)); // 单位阵不是有效的旋转矩阵
        if (ctx->hasMovieDisplayMatrix) {
            MEDIA_LOG_D("Movie has non-identity display matrix");
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseMvhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_ONE, Status::ERROR_INVALID_DATA, "Invalid mvhd depth");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_MVHD_SIZE && currentAtom.dataSize <= MAX_DATA_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid atom size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    auto headerInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(headerInfo != nullptr, Status::ERROR_NO_MEMORY, "Allocate mvhd header info failed");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read mvhd header info failed");
    MvhdInfo mvhdInfo = ParseMvhdInfo(headerInfo.get(), currentAtom.dataSize);
    SetMvhdMediaInfo(mvhdInfo);
    return ParseMvhdDisplayMatrix(currentAtom, headerInfo.get(), ctx, currentAtom.dataSize);
}

int64_t MPEG4AtomParser::GetElstDuration(std::shared_ptr<Track> track)
{
    int32_t fileTimescale = movieTimeScale_;
    FALSE_RETURN_V_MSG_E(fileTimescale > 0, 0, "Invalid file time scale");
    int32_t trackTimescale = track->timeScale;
    FALSE_RETURN_V_MSG_E(trackTimescale > 0, 0, "Invalid track time scale");
    int64_t elstDuration = 0;
    MEDIA_LOG_D("File timescale: " PUBLIC_LOG_D32 ", track timescale: " PUBLIC_LOG_D32,
        fileTimescale, trackTimescale);
    MEDIA_LOG_D("Total elst duration: " PUBLIC_LOG_D64, track->totalElstDuration);
    if (track->totalElstDuration && fileTimescale && trackTimescale) {
        ConvertTimeScale(track->totalElstDuration, trackTimescale, fileTimescale, &elstDuration);
        MEDIA_LOG_D("Using elst duration for track " PUBLIC_LOG_U32 ": " PUBLIC_LOG_D64,
            track->trackIndex, elstDuration);
    }
    return elstDuration;
}

int64_t MPEG4AtomParser::GetTrackDuration(std::shared_ptr<Track> track, bool calculateBitrate)
{
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper && IsValidTrackIndex(mediaInfo_, track->trackIndex), 0,
        "Invalid track for duration calculation");

    int64_t duration = 0;
    if (track->sampleHelper && !track->sampleHelper->IsSampleIndexEmpty()) {
        if (calculateBitrate) {
            duration = track->sampleHelper->totalDuration_;
            MEDIA_LOG_D("Using raw sample helper duration for bitrate: " PUBLIC_LOG_D64, duration);
            return duration;
        }
        if (hasMoofBox_) {
            duration = track->sampleHelper->totalDuration_;
        } else {
            duration = std::min(track->duration, track->sampleHelper->totalDuration_);
        }
        MEDIA_LOG_D("sample helper duration" PUBLIC_LOG_D64, track->sampleHelper->totalDuration_);
    }
    if (track->totalElstDuration) {
        int64_t elstDuration = GetElstDuration(track);
        MEDIA_LOG_D("Elst duration for track " PUBLIC_LOG_U32 ": " PUBLIC_LOG_D64,
            track->trackIndex, elstDuration);
        if (elstDuration > 0) {
            duration = std::min(duration, elstDuration);
        }
    }

    // 从trun中累加 （适用于FMP4文件）
    if (hasMoofBox_ && track->sampleHelper) {
        duration = track->sampleHelper->totalTrunDuration_;
        MEDIA_LOG_D("Using fragment duration for track " PUBLIC_LOG_U32 ": " PUBLIC_LOG_D64
            " (fragments: " PUBLIC_LOG_ZU ")", track->trackIndex, duration, fragmentEntry_.size());
    }

    if (duration > 0 && duration < INT64_MAX) {
        return duration;
    } else {
        MEDIA_LOG_W("Cannot get valid duration for track " PUBLIC_LOG_U32, track->trackIndex);
        return -1;  // 返回-1表示无法获取有效的持续时间
    }
}

int64_t CalculateAverageBitrate(int64_t bitsInTimeScale, int64_t duration)
{
    int64_t safeDuration = duration;
    if (safeDuration <= 0) {
        return 0;
    }
    return bitsInTimeScale / safeDuration;
}

int64_t GetEffectiveSampleCount(const std::shared_ptr<MPEG4SampleHelper>& sampleHelper)
{
    FALSE_RETURN_V_NOLOG(sampleHelper != nullptr, 0);
    int64_t rawSampleCount = sampleHelper->GetRawSampleCount();
    return rawSampleCount > 0 ? rawSampleCount : static_cast<int64_t>(sampleHelper->GetSampleCount());
}

void MPEG4AtomParser::CalculateTrackBitrates(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr && track->sampleHelper != nullptr, "Invalid track or sample helper");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");
    int64_t duration = 0;
    int64_t totalSize = 0;
    if (!GetBitrateDurationAndSize(track, duration, totalSize)) {
        return;
    }
    int32_t timeScale = track->timeScale;
    FALSE_RETURN_MSG_W(timeScale > 0, "Track " PUBLIC_LOG_U32 " has invalid time scale", track->trackIndex);
    int64_t bitsPerTimeScale = 0;
    if (__builtin_mul_overflow(static_cast<int64_t>(BYTE_TO_BITS), static_cast<int64_t>(timeScale),
        &bitsPerTimeScale)) {
        MEDIA_LOG_E("Mul overflow: [" PUBLIC_LOG_D32 "," PUBLIC_LOG_D32 "]", BYTE_TO_BITS, timeScale);
        return;
    }
    int64_t bitrate = 0;
    if (!hasMoofBox_) {
        int64_t temp = 0;
        if (__builtin_mul_overflow(totalSize, bitsPerTimeScale, &temp)) {
            MEDIA_LOG_E("Calculate bitrate overflow: [" PUBLIC_LOG_D64 "," PUBLIC_LOG_D64 "]",
                totalSize, bitsPerTimeScale);
            return;
        }
        bitrate = CalculateAverageBitrate(temp, duration);
    } else {
        if (ConvertTimeScaleRoundToTimeBase(totalSize, bitsPerTimeScale, duration, &bitrate) != Status::OK) {
            MEDIA_LOG_E("Calculate bitrate overflow: [" PUBLIC_LOG_D64 "," PUBLIC_LOG_D64 "," PUBLIC_LOG_D64 "]",
                totalSize, bitsPerTimeScale, duration);
            return;
        }
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_BITRATE>(bitrate);
}

int64_t MPEG4AtomParser::GetBitrateDuration(const std::shared_ptr<Track>& track)
{
    if (!hasMoofBox_) {
        int64_t rawDuration = track->sampleHelper->GetRawTotalDuration();
        if (track->duration > 0 && rawDuration > 0) {
            return std::min(track->duration, rawDuration);
        }
        return rawDuration > 0 ? rawDuration : GetTrackDuration(track, true);
    }
    int64_t rawDuration = track->sampleHelper->GetRawTotalDuration();
    return rawDuration > 0 ? rawDuration : GetTrackDuration(track, true);
}

bool MPEG4AtomParser::HasEnoughSamplesForBitrate(const std::shared_ptr<Track>& track, const std::string& mime)
{
    int64_t totalSampleCount = GetEffectiveSampleCount(track->sampleHelper);
    if (totalSampleCount <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has no samples, skip calculated bitrate", track->trackIndex);
        return false;
    }
    FALSE_RETURN_V_MSG_W(
        !IsVideoMimeType(mime) || track->sampleHelper->GetRawSampleCount() > 0, false,
        "Video track " PUBLIC_LOG_U32 " has no raw samples, skip calculated bitrate", track->trackIndex);
    return true;
}

int64_t MPEG4AtomParser::GetBitrateTotalSize(const std::shared_ptr<Track>& track)
{
    int64_t totalSize = track->sampleHelper->GetRawTotalSize();
    if (totalSize > 0) {
        return totalSize;
    }
    for (uint32_t i = 0; i < track->sampleHelper->GetSampleCount(); ++i) {
        int32_t size = 0;
        Status ret = track->sampleHelper->GetSampleSize(i, size);
        if (ret != Status::OK) {
            MEDIA_LOG_E("Get sample size failed when calculating bitrate total size, index=" PUBLIC_LOG_U32
                ", count=" PUBLIC_LOG_ZU ", ret=" PUBLIC_LOG_D32,
                i, track->sampleHelper->GetSampleCount(), static_cast<int32_t>(ret));
            return 0;
        }
        int64_t newTotalSize = 0;
        if (__builtin_add_overflow(totalSize, static_cast<int64_t>(size), &newTotalSize)) {
            MEDIA_LOG_E("GetBitrateTotalSize overflow");
            return 0;
        }
        totalSize = newTotalSize;
    }
    return totalSize;
}

bool MPEG4AtomParser::GetBitrateDurationAndSize(std::shared_ptr<Track> track, int64_t& duration, int64_t& totalSize)
{
    FALSE_RETURN_V_MSG_E(track != nullptr && track->sampleHelper != nullptr, false, "Invalid track or sample helper");
    FALSE_RETURN_V_MSG_E(IsValidTrackIndex(mediaInfo_, track->trackIndex), false, "Invalid track index");
    duration = GetBitrateDuration(track);
    totalSize = GetBitrateTotalSize(track);
    if (!HasEnoughSamplesForBitrate(track, track->sampleHelper->mimeType_)) {
        return false;
    }
    if (hasSidxBox_ && track->sidxDuration > 0 && track->sidxReferencedSize > 0) {
        duration = track->sidxDuration;
        totalSize = track->sidxReferencedSize;
    }
    FALSE_RETURN_V_MSG_W(
        duration > 0 && totalSize > 0, false,
        "Track " PUBLIC_LOG_U32 " cannot calculate bitrate from samples", track->trackIndex);
    return true;
}

void MPEG4AtomParser::CalculateVideoFrameRate(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr && track->sampleHelper != nullptr, "Invalid track or sample helper");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");
    // get track time scale and duration
    int32_t timeScale = track->timeScale;
    FALSE_LOG_MSG_W(timeScale > 0, "Track " PUBLIC_LOG_U32 " has invalid time scale", track->trackIndex);

    int64_t duration = track->sampleHelper->GetRawTotalDuration() > 0 ?
        track->sampleHelper->GetRawTotalDuration() : GetTrackDuration(track);
    int64_t totalSamples = GetEffectiveSampleCount(track->sampleHelper);
    if (totalSamples <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has invalid sample count", track->trackIndex);
        if (timeScale > 0) {
            mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_FRAME_RATE>(static_cast<double>(timeScale));
        }
        return;
    }
    // 计算原始帧率
    if (duration > 0) {
        double rawFrameRate = static_cast<double>(timeScale) * totalSamples / duration;
        mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_FRAME_RATE>(rawFrameRate);
        MEDIA_LOG_D("Track:" PUBLIC_LOG_U32 ", totalSamples=" PUBLIC_LOG_D64 ", duration=" PUBLIC_LOG_D64
            ", timeScale=" PUBLIC_LOG_D32 ", frameRate=" PUBLIC_LOG_F " fps", track->trackIndex, totalSamples,
            duration, timeScale, rawFrameRate);
    } else if (timeScale > 0) {
        mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_FRAME_RATE>(
            std::numeric_limits<double>::infinity());
        MEDIA_LOG_D("Track:" PUBLIC_LOG_U32 ", totalSamples=" PUBLIC_LOG_D64
            ", duration=" PUBLIC_LOG_D64 ", frameRate=inf", track->trackIndex, totalSamples, duration);
    }
}

void MPEG4AtomParser::CalculateVideoDelay(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr && track->sampleHelper != nullptr, "Invalid track or sample helper");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");
    uint32_t videoDelay = 0;
    if (track->sampleHelper->mimeType_ == MimeType::VIDEO_AVC) {
        if (!track->sampleHelper->cttsEntry_.empty()) {
            videoDelay = track->sampleHelper->EstimateVideoDelay();
            if (videoDelay == 0) {
                constexpr uint32_t avcDefaultReorderDelay = 2;
                videoDelay = avcDefaultReorderDelay;
            }
        } else if (track->codecParms.data != nullptr && track->codecParms.extradataSize > 0) {
            AvcParserImpl avcParser;
            int64_t sampleCount = GetEffectiveSampleCount(track->sampleHelper);
            if (avcParser.ParseMaxReorderPicFromAvcConfig(
                track->codecParms.data.get(), track->codecParms.extradataSize) &&
                sampleCount > static_cast<int64_t>(avcParser.GetMaxReorderPic() + 1)) {
                videoDelay = avcParser.GetMaxReorderPic();
            }
        }
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_DELAY>(videoDelay);
}

// LCOV_EXCL_START
AudioSampleFormat MPEG4AtomParser::GetAv3aSampleFormat(int32_t bitsPerRawSample) const
{
    FALSE_RETURN_V_NOLOG(bitsPerRawSample != AUDIO_BITS_S16, AudioSampleFormat::SAMPLE_S16LE);
    FALSE_RETURN_V_NOLOG(bitsPerRawSample != AUDIO_BITS_S24, AudioSampleFormat::SAMPLE_S24LE);
    FALSE_RETURN_V_NOLOG(bitsPerRawSample != AUDIO_BITS_U8, AudioSampleFormat::SAMPLE_U8);
    return AudioSampleFormat::INVALID_WIDTH;
}

AudioChannelLayout MPEG4AtomParser::GetAv3aChannelLayout(int32_t channelCount, int32_t sampleRate) const
{
    FALSE_RETURN_V_NOLOG(channelCount != AV3A_STEREO_CHANNELS || sampleRate != AV3A_STEREO_SAMPLE_RATE,
        AudioChannelLayout::STEREO);
    FALSE_RETURN_V_NOLOG(channelCount != AV3A_5POINT1POINT4_CHANNELS &&
        channelCount != AV3A_LEGACY_5POINT1POINT4_CHANNELS, AudioChannelLayout::CH_5POINT1POINT4);
    FALSE_RETURN_V_NOLOG(channelCount != AV3A_7POINT1POINT4_CHANNELS, AudioChannelLayout::CH_7POINT1POINT4);
    FALSE_RETURN_V_NOLOG(channelCount != AV3A_HOA_ORDER3_CHANNELS, AudioChannelLayout::HOA_ORDER3_ACN_SN3D);
    return AudioChannelLayout::UNKNOWN;
}
// LCOV_EXCL_STOP

void MPEG4AtomParser::NormalizeAudioAttributes(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr, "Invalid track");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");

    std::string mime;
    FALSE_RETURN_MSG(mediaInfo_.tracks[track->trackIndex].Get<Tag::MIME_TYPE>(mime), "Audio track has no mime");
    bool keepSamplePerFrame = false;
    if (mime != MimeType::AUDIO_AAC) {
        mediaInfo_.tracks[track->trackIndex].Remove(Tag::AUDIO_AAC_IS_ADTS);
    }
    if (mime == MimeType::AUDIO_AAC) {
        int32_t samplesPerFrame = 0;
        keepSamplePerFrame = mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame) &&
            samplesPerFrame == AUDIO_DEFAULT_SAMPLE_PER_FRAME;
    } else if (mime == MimeType::AUDIO_AVS3DA) {
        AudioSampleFormat sampleFormat = AudioSampleFormat::INVALID_WIDTH;
        if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat)) {
            mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::INVALID_WIDTH);
        }
        AudioChannelLayout channelLayout = AudioChannelLayout::UNKNOWN;
        if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout)) {
            mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::UNKNOWN);
        }
        AudioChannelLayout outputChannelLayout = AudioChannelLayout::UNKNOWN;
        if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(outputChannelLayout)) {
            mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::UNKNOWN);
        }
        return;
    } else if (mime == MimeType::AUDIO_MPEG && track->audioSampleEntryChannelCount > 0) {
        mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_CHANNEL_COUNT>(
            static_cast<int32_t>(track->audioSampleEntryChannelCount));
        mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNELS>(
            static_cast<int32_t>(track->audioSampleEntryChannelCount));
        AudioChannelLayout layout = Converter::GetDefaultChannelLayout(track->audioSampleEntryChannelCount);
        mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
        mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
    }
    if (!keepSamplePerFrame) {
        mediaInfo_.tracks[track->trackIndex].Remove(Tag::AUDIO_SAMPLE_PER_FRAME);
    }
}

void MPEG4AtomParser::ParseImageTrackData(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr && track->sampleHelper != nullptr, "Invalid track");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");

    if (track->sampleHelper->IsSampleIndexEmpty()) {
        MEDIA_LOG_W("Image track " PUBLIC_LOG_U32 " has no sample entries", track->trackIndex);
        return;
    }
    MPEG4SampleHelper::SampleIndexView firstEntry;
    FALSE_RETURN_MSG(track->sampleHelper->GetSampleIndexView(0, firstEntry) == Status::OK,
        "Get image track sample failed");
    FALSE_RETURN_MSG_W(
        firstEntry.size > 0 && firstEntry.pos >= 0,
        "Image track " PUBLIC_LOG_U32 " first sample invalid", track->trackIndex);
    FALSE_RETURN_MSG_W(static_cast<uint64_t>(firstEntry.size) <= MPEG4_MAX_BOX_DATA_SIZE,
        "Image track " PUBLIC_LOG_U32 " first sample too large", track->trackIndex);

    auto coverData = std::make_unique<uint8_t[]>(firstEntry.size);
    FALSE_RETURN_MSG(coverData != nullptr, "Failed to allocate cover data buffer");
    Status ret = dataReader_->ReadUintData(static_cast<uint64_t>(firstEntry.pos),
        coverData.get(), static_cast<uint64_t>(firstEntry.size));
    FALSE_RETURN_MSG(ret == Status::OK, "Read image track data failed");

    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    if (track->sampleHelper->mimeType_ == MimeType::IMAGE_JPG) {
        ParseJpegDimensions(coverData.get(), firstEntry.size, videoWidth, videoHeight);
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_WIDTH>(videoWidth);
    mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_HEIGHT>(videoHeight);

    std::vector<uint8_t> cover(firstEntry.size);
    cover.assign(coverData.get(), coverData.get() + firstEntry.size);
    mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_COVER>(std::move(cover));
    MEDIA_LOG_D("Image track " PUBLIC_LOG_U32 " cover data size " PUBLIC_LOG_D32,
        track->trackIndex, firstEntry.size);
}

Status ConvertRoundedTimeScale(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result)
{
    if (result == nullptr || timeScale == 0 || timeScale < 0) {
        return Status::ERROR_INVALID_DATA;
    }
    const int64_t checkedTimeScale = timeScale;

    int64_t temp = 0;
    if (__builtin_mul_overflow(multiplicand, multiplier, &temp)) {
        return Status::ERROR_INVALID_DATA;
    }
    int64_t quotient = temp / checkedTimeScale;
    int64_t remainder = temp % checkedTimeScale;
    int64_t halfTimeScale = checkedTimeScale / ROUND_HALF_DIVISOR;
    int64_t roundThreshold = checkedTimeScale - halfTimeScale;
    if (remainder > 0 && remainder >= roundThreshold) {
        FALSE_RETURN_V_MSG_E(quotient < std::numeric_limits<int64_t>::max(),
            Status::ERROR_INVALID_DATA, "Round timescale overflow");
        ++quotient;
    } else if (remainder < 0 && -remainder >= roundThreshold) {
        FALSE_RETURN_V_MSG_E(quotient > std::numeric_limits<int64_t>::min(),
            Status::ERROR_INVALID_DATA, "Round timescale underflow");
        --quotient;
    }
    *result = quotient;
    return Status::OK;
}

Status ConvertTimeScaleRound(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result)
{
    return ConvertRoundedTimeScale(multiplicand, multiplier, timeScale, result);
}

Status ConvertTimeScaleRoundToTimeBase(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result)
{
    return ConvertRoundedTimeScale(multiplicand, multiplier, timeScale, result);
}

Status ConvertTrackTimeToUs(int64_t trackTime, int64_t trackTimeScale, int64_t* result)
{
    constexpr int64_t timeBase = 10000000;
    constexpr int64_t timeBasePerUs = 10;
    int64_t tempTime = 0;
    FALSE_RETURN_V_MSG_E(ConvertTimeScaleRoundToTimeBase(trackTime, timeBase, trackTimeScale, &tempTime) ==
        Status::OK, Status::ERROR_INVALID_DATA, "Convert to time base failed");
    *result = tempTime / timeBasePerUs;
    return Status::OK;
}

void MPEG4AtomParser::SetCodecConfig(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr, "track is null");
    if (track->codecParms.extradataSize > 0 && track->codecParms.data != nullptr) {
        std::vector<uint8_t> extra(track->codecParms.extradataSize);
        extra.assign(track->codecParms.data.get(), track->codecParms.data.get() + track->codecParms.extradataSize);
        mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_CODEC_CONFIG>(extra);
        track->codecParms.annexbExtradataSize = track->codecParms.extradataSize;
        track->codecParms.hasValidExtradata = true;
    } else {
        MEDIA_LOG_E("Set codec config failed");
    }
}

Status MPEG4AtomParser::InitCurrentTrackForTrak()
{
    auto track = std::make_shared<Track>();
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate memory for track");
    track->cache = std::make_shared<SampleCache>();
    FALSE_RETURN_V_MSG_E(track->cache != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate track cache");
    size_t originalTrackCount = mediaInfo_.tracks.size();
    Meta newTrackMeta;
    mediaInfo_.tracks.emplace_back(std::move(newTrackMeta));
    if (mediaInfo_.tracks.size() != originalTrackCount + 1) {
        return Status::ERROR_NO_MEMORY;
    }
    if (lastTrack_) {
        lastTrack_->next = track;
    } else {
        firstTrack_ = track;
    }
    lastTrack_ = track;
    lastTrack_->trackIndex = trackCount_;
    ++trackCount_;
    lastTrack_->sampleHelper = std::make_shared<MPEG4SampleHelper>();
    FALSE_RETURN_V_MSG_E(lastTrack_->sampleHelper != nullptr, Status::ERROR_NO_MEMORY,
        "Failed create sample helper");
    lastTrack_->sampleHelper->seekable_ = seekable_;
    lastTrack_->sampleHelper->dataReader_ = dataReader_;
    lastTrack_->sampleHelper->SetFragmented(hasMoofBox_);
    return Status::OK;
}

void MPEG4AtomParser::UpdateParsedTrackVideoInfo()
{
    if (NeedParseVideoInfo(lastTrack_->sampleHelper->mimeType_, lastTrack_->codecParms.trackType)) {
        ParseOrientationFromMatrix(lastTrack_);
        ParseRotationTypeFromMatrix(lastTrack_);
        CalculateSARFromTrackInfo(lastTrack_);
    }
}

Status MPEG4AtomParser::FinalizeParsedTrackSampleTables()
{
    if (lastTrack_->sampleHelper) {
        int32_t movieTimeScale = 0;
        int32_t trackTimeScale = 0;
        if (!lastTrack_->editListEntries.empty()) {
            movieTimeScale = movieTimeScale_;
            trackTimeScale = lastTrack_->timeScale;
            lastTrack_->sampleHelper->SetEditListParams(
                lastTrack_->editListEntries, movieTimeScale, trackTimeScale);
        }
        Status finalizeRet = lastTrack_->sampleHelper->FinalizeSampleTables();
        if (finalizeRet != Status::OK) {
            MEDIA_LOG_E("Finalize sample tables failed");
            return finalizeRet;
        }
        if (lastTrack_->sampleHelper->ShouldBuildDenseIndex()) {
            Status denseRet = lastTrack_->sampleHelper->BuildSampleIndexEntries();
            FALSE_RETURN_V_MSG_E(denseRet == Status::OK, denseRet, "Build dense sample index failed");
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseTrak(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_ONE, Status::ERROR_INVALID_DATA, "Invalid trak depth");
    Status ret = InitCurrentTrackForTrak();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to init track");
    ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse trak atom");
    UpdateParsedTrackVideoInfo();
    ret = FinalizeParsedTrackSampleTables();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Failed to finalize sample tables");
        pendingSampleTableStatus_ = ret;
        // Fragmented MP4 may keep empty placeholder sample tables in moov and be rejected later by
        // mvex/moof/sidx as unsupported. Defer this error so the plugin manager can still fallback.
        return Status::OK;
    }
    SetCodecConfig(lastTrack_);
    AdapterFormat(lastTrack_);
    return Status::OK;
}

void MPEG4AtomParser::AdapterFormat(std::shared_ptr<Track> track)
{
    int32_t channelCount = -1;
    if (mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount)) {
        mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNELS>(channelCount);
    }
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    if (mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout)) {
        mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
    }

    // 辅助轨相关处理
    if (track->codecParms.trackType != TrackType::AUXILIARY_TYPE &&
        track->codecParms.trackType != TrackType::TIMEDMETA_TYPE) {
        mediaInfo_.tracks[track->trackIndex].Remove(Tag::TRACK_DESCRIPTION);
        mediaInfo_.tracks[track->trackIndex].Remove(Tag::TRACK_REFERENCE_TYPE);
        mediaInfo_.tracks[track->trackIndex].Remove(Tag::REFERENCE_TRACK_IDS);

        mediaInfo_.tracks[track->trackIndex].Remove(Tag::TIMED_METADATA_KEY);
        mediaInfo_.tracks[track->trackIndex].Remove(Tag::TIMED_METADATA_SRC_TRACK);
    }
}
void MPEG4AtomParser::CalculateSampleAspectRatio(std::shared_ptr<Track> track, uint32_t width, uint32_t height)
{
    FALSE_RETURN_MSG(track != nullptr, "CalculateSampleAspectRatio: track is null");
    FALSE_RETURN_MSG(track->hasDisplayMatrix && track->displayMatrix != nullptr, "Track has no valid display matrix");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");

    constexpr int32_t transformComponents = 2; // 变换组件数量：X和Y
    constexpr int32_t matrixColOffset = 3;     // 矩阵列偏移量
    std::array<double, transformComponents> dispTransform;

    for (int32_t i = 0; i < transformComponents; ++i) {
        double x = static_cast<double>(track->displayMatrix[0 + i]) / CONVERT_SCALE;
        double y = static_cast<double>(track->displayMatrix[matrixColOffset + i]) / CONVERT_SCALE;
        dispTransform[i] = std::hypot(x, y);
    }
    constexpr double minTransform = 1.0;
    constexpr double maxTransform = 1 << 24;
    constexpr double sarThreshold = 0.01;

    if (dispTransform[0] > minTransform && dispTransform[1] > minTransform &&
        dispTransform[0] < maxTransform && dispTransform[1] < maxTransform &&
        fabs((dispTransform[0] / dispTransform[1]) - minTransform) > sarThreshold) {
        double sar = dispTransform[0] / dispTransform[1];
        mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_SAR>(sar);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " SAR calculation skip", track->trackIndex);
    }
}

void MPEG4AtomParser::CalculateSARFromTrackInfo(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr, "CalculateSARFromTrackInfo: track is null");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");

    double sar = 0.0;

    if (track->hasPasp && track->hSpacing > 0 && track->vSpacing > 0) {
        sar = static_cast<double>(track->hSpacing) / static_cast<double>(track->vSpacing);
        MEDIA_LOG_D("SAR from pasp: hSpacing=%{public}u, vSpacing=%{public}u, sar=%{public}f",
            track->hSpacing, track->vSpacing, sar);
        mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_SAR>(sar);
        return;
    }

    if (track->stsdWidth > 0 && track->stsdHeight > 0 &&
        track->tkhdWidth > 0 && track->tkhdHeight > 0 &&
        (track->stsdWidth != track->tkhdWidth || track->stsdHeight != track->tkhdHeight)) {
        uint64_t numerator = 0;
        uint64_t denominator = 0;
        if (__builtin_mul_overflow(static_cast<uint64_t>(track->stsdHeight), track->tkhdWidth, &numerator)) {
            MEDIA_LOG_E("Calculate SAR overflow: track->stsdHeight * track->tkhdWidth");
            return;
        }
        if (__builtin_mul_overflow(static_cast<uint64_t>(track->stsdWidth), track->tkhdHeight, &denominator)) {
            MEDIA_LOG_E("Calculate SAR overflow: track->stsdWidth * track->tkhdHeight");
            return;
        }
        if (denominator > 0) {
            sar = static_cast<double>(numerator) / static_cast<double>(denominator);
            MEDIA_LOG_D("SAR from width/height diff: [%{public}u/%{public}u/%{public}u/%{public}u], sar=%{public}f",
                track->stsdWidth, track->stsdHeight, track->tkhdWidth, track->tkhdHeight, sar);
            mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_SAR>(sar);
            return;
        }
    }
    MEDIA_LOG_D("Track %{public}u SAR: no pasp and no width/height diff, skip", track->trackIndex);
}

void MPEG4AtomParser::SetTrackDisplayMatrix(std::shared_ptr<Track> track, const int32_t resultMatrix[3][3],
    uint32_t width, uint32_t height, const size_t len)
{
    if (!IsDisplayMatrixIdentity(resultMatrix, len)) {
        constexpr uint16_t matrixSize = 9;
        track->displayMatrix = std::make_unique<int32_t[]>(matrixSize);
        FALSE_RETURN_MSG(track->displayMatrix != nullptr, "Failed to allocate memory for display matrix");
        for (uint16_t i = 0; i < DISPLAY_MATRIX_ROWS; ++i) {
            for (uint16_t j = 0; j < DISPLAY_MATRIX_COLS; ++j) {
                track->displayMatrix[i * DISPLAY_MATRIX_COLS + j] = resultMatrix[i][j];
            }
        }
        track->hasDisplayMatrix = true;
        if (width && height) {
            CalculateSampleAspectRatio(track, width, height);
        }
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has identity display matrix, skipping", track->trackIndex);
        track->hasDisplayMatrix = false;
    }
}

Status MPEG4AtomParser::ParseTkhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_TKHD_SIZE, Status::ERROR_INVALID_DATA,
        "Invalid tkhd atom size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    /*
     * tkhd的所有版本中最大的值
     * creation(8) + modification(8) + trackId(4) + reserved(4) + duration(8) + reserved(8) +
     * layer(2) + alternate_group(2) + volume(2) + reserved(2) + matrix(36) + width(4) + height(4)
    */
    constexpr uint32_t infoSize = 96;
    auto headerInfo = std::make_unique<uint8_t[]>(infoSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo.get(), infoSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tkhd header info failed");
    uint8_t version = headerInfo[0];

    int64_t dynamicSize = 0;
    if (version == 1) {
        // skip some unneeded parameters
        lastTrack_->trackId = static_cast<int32_t>(GetU32Value(&headerInfo[OFFSET_V1]));
        dynamicSize = DYNAMIC_SIZE_V1;
    } else if (version == 0) {
        lastTrack_->trackId = static_cast<int32_t>(GetU32Value(&headerInfo[OFFSET_V0]));
        dynamicSize = DYNAMIC_SIZE_V0;
    } else {
        MEDIA_LOG_E("Invalid tkhd version");
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t width = GetU32Value(&headerInfo[dynamicSize + 52]) >> 16;
    uint32_t height = GetU32Value(&headerInfo[dynamicSize + 56]) >> 16;
    lastTrack_->tkhdWidth = width;
    lastTrack_->tkhdHeight = height;
    // 解析track level display matrix
    int64_t matrixOffset = dynamicSize + 16; // track matrix在width/height之前
    if (currentAtom.dataSize >= static_cast<uint64_t>(matrixOffset + DISPLAY_MATRIX_SIZE)) {
        int32_t trackDisplayMatrix[DISPLAY_MATRIX_ROWS][DISPLAY_MATRIX_COLS];
        ret = ParseDisplayMatrix(&headerInfo[matrixOffset], trackDisplayMatrix);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse track display matrix");
        // 计算最终的display matrix (track * movie)
        int32_t resultMatrix[DISPLAY_MATRIX_ROWS][DISPLAY_MATRIX_COLS];
        if (ctx->hasMovieDisplayMatrix) {
            MultiplyDisplayMatrixDataLen len{sizeof(trackDisplayMatrix),
                                             sizeof(ctx->movieDisplayMatrix), sizeof(resultMatrix)};
            MultiplyDisplayMatrix(trackDisplayMatrix, ctx->movieDisplayMatrix, resultMatrix, len);
        } else {
            FALSE_RETURN_V_MSG_E(
                memcpy_s(&resultMatrix[0][0], sizeof(int32_t) * DISPLAY_MATRIX_ROWS * DISPLAY_MATRIX_COLS,
                &trackDisplayMatrix[0][0], sizeof(int32_t) * DISPLAY_MATRIX_ROWS * DISPLAY_MATRIX_COLS) == EOK,
                Status::ERROR_INVALID_DATA, "Failed to copy track display matrix");
        }
        SetTrackDisplayMatrix(lastTrack_, resultMatrix, width, height, sizeof(resultMatrix));
    } else {
        MEDIA_LOG_E("tkhd atom too small for display matrix, size: " PUBLIC_LOG_U64, currentAtom.dataSize);
        lastTrack_->hasDisplayMatrix = false;
    }
    return ret;
}

Status MPEG4AtomParser::ParseEdts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse edts container failed");
    return ret;
}

int64_t GetSignedU64Value(const uint8_t* data)
{
    uint64_t value = GetU64Value(data);
    FALSE_RETURN_V_NOLOG(value > static_cast<uint64_t>(INT64_MAX), static_cast<int64_t>(value));
    FALSE_RETURN_V_NOLOG(value != (static_cast<uint64_t>(1) << INT64_SIGN_BIT_SHIFT), INT64_MIN);
    return -static_cast<int64_t>((~value) + 1);
}

struct ElstEntryInfo {
    int64_t segmentDuration = 0;
    int64_t mediaTime = 0;
    int16_t mediaRateInteger = 1;
    int16_t mediaRateFraction = 0;
};

Status ParseElstEntry(const uint8_t* elstInfo, uint32_t index, uint8_t version, ElstEntryInfo& entryInfo)
{
    constexpr uint32_t elstHeaderSize = 8;
    constexpr uint32_t entrySizeV0 = 12;
    constexpr uint32_t entrySizeV1 = 20;
    uint32_t entrySize = (version == 1) ? entrySizeV1 : entrySizeV0;
    uint32_t entryOffset = elstHeaderSize + index * entrySize;
    if (version == 1) {
        entryInfo.segmentDuration = static_cast<int64_t>(GetU64Value(&elstInfo[entryOffset]));
        entryInfo.mediaTime = GetSignedU64Value(&elstInfo[entryOffset + sizeof(uint64_t)]);
    } else {
        uint32_t tempDuration = GetU32Value(&elstInfo[entryOffset]);
        int32_t tempMediaTime = static_cast<int32_t>(GetU32Value(&elstInfo[entryOffset + sizeof(uint32_t)]));
        entryInfo.segmentDuration = tempDuration;
        entryInfo.mediaTime = tempMediaTime;
    }
    uint32_t rateOffset = entryOffset + (version == 1 ? sizeof(uint64_t) * 2 : sizeof(uint32_t) * 2);
    entryInfo.mediaRateInteger = static_cast<int16_t>(GetU16Value(&elstInfo[rateOffset]));
    entryInfo.mediaRateFraction = static_cast<int16_t>(GetU16Value(&elstInfo[rateOffset + sizeof(uint16_t)]));
    MEDIA_LOG_D("Parsed elst entry " PUBLIC_LOG_U32 ": segmentDuration=" PUBLIC_LOG_D64 ", mediaTime=" PUBLIC_LOG_D64,
        index, entryInfo.segmentDuration, entryInfo.mediaTime);
    FALSE_RETURN_V_MSG_E(entryInfo.mediaTime >= 0 || entryInfo.mediaTime == -1, Status::ERROR_INVALID_DATA,
        "Invalid edit list media time: " PUBLIC_LOG_D64, entryInfo.mediaTime);

    return Status::OK;
}

Status AppendElstEntry(const uint8_t* elstInfo, uint32_t index, uint8_t version,
    const std::shared_ptr<MPEG4AtomParser::Track>& track, bool& foundFirstMediaEdit)
{
    ElstEntryInfo entryInfo;
    Status ret = ParseElstEntry(elstInfo, index, version, entryInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse elst entry failed");
    FALSE_RETURN_V_MSG_E(entryInfo.segmentDuration >= 0, Status::ERROR_INVALID_DATA,
        "Invalid elst segment duration: " PUBLIC_LOG_D64, entryInfo.segmentDuration);
    int64_t totalElstDuration = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(track->totalElstDuration, entryInfo.segmentDuration,
        &totalElstDuration), Status::ERROR_INVALID_DATA, "Elst total duration overflow");
    track->totalElstDuration = totalElstDuration;
    track->editListEntries.emplace_back(MPEG4SampleHelper::EditListEntry {
        .mediaTime = entryInfo.mediaTime,
        .duration = entryInfo.segmentDuration,
        .mediaRateInteger = entryInfo.mediaRateInteger,
        .mediaRateFraction = entryInfo.mediaRateFraction
    });
    if (index == 0 && entryInfo.mediaTime == -1) {
        track->elstInitEmptyEdit = entryInfo.segmentDuration;
        return Status::OK;
    }
    if (!foundFirstMediaEdit && entryInfo.mediaTime >= 0) {
        track->elstShiftStartTime = entryInfo.mediaTime;
        foundFirstMediaEdit = true;
    }
    return Status::OK;
}

Status ParseElstEntries(const uint8_t* elstInfo, uint32_t entryCount, uint8_t version,
    const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    track->editListEntries.clear();
    bool foundFirstMediaEdit = false;
    for (uint32_t i = 0; i < entryCount; ++i) {
        Status ret = AppendElstEntry(elstInfo, i, version, track, foundFirstMediaEdit);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Append elst entry failed");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseElst(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_ELST_SIZE, Status::ERROR_INVALID_DATA,
        "Invalid elst atom size");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "elst"), Status::ERROR_INVALID_DATA,
        "Invalid elst data size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    auto elstInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, elstInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read elst info failed");
    uint8_t version = elstInfo[0];
    FALSE_RETURN_V_MSG_E(version == 0 || version == 1, Status::ERROR_INVALID_DATA, "Invalid elst version");
    uint32_t entryCount = GetU32Value(&elstInfo[4]);
    FALSE_RETURN_V_MSG_E(entryCount > 0, Status::ERROR_INVALID_DATA,
        "Invalid entry count: " PUBLIC_LOG_U32, entryCount);
    constexpr uint32_t elstHeaderSize = 8;
    constexpr uint32_t entrySizeV0 = 12;
    constexpr uint32_t entrySizeV1 = 20;
    uint32_t entrySize = (version == 1) ? entrySizeV1 : entrySizeV0;
    FALSE_RETURN_V_MSG_E((currentAtom.dataSize - elstHeaderSize) / entrySize >= entryCount,
        Status::ERROR_INVALID_DATA, "Invalid elst data size");
    return ParseElstEntries(elstInfo.get(), entryCount, version, lastTrack_);
}

Status MPEG4AtomParser::ParseTref(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_TWO, Status::ERROR_INVALID_DATA, "Invalid tref depth");
    constexpr uint32_t typeOffset = 4;
    constexpr uint32_t minAtomSize = 8;
    uint8_t subAtomData[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, subAtomData, sizeof(subAtomData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tref data failed");
    uint32_t subAtomSize = GetU32Value(&subAtomData[0]);
    FALSE_RETURN_V_MSG_E(subAtomSize >= minAtomSize, ret, "Invalid atom size");
    uint32_t subAtomType = GetU32Value(&subAtomData[typeOffset]);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::TRACK_REFERENCE_TYPE>(FourccToString(subAtomType));

    MEDIA_LOG_D("ParseTref subAtomSize: " PUBLIC_LOG_U32 " subAtomType: " PUBLIC_LOG_U32 ", type: " PUBLIC_LOG_S,
        subAtomSize, subAtomType, FourccToString(subAtomType).c_str());

    uint32_t idCount = (subAtomSize - 8) / 4;
    FALSE_RETURN_V_MSG_E(idCount > 0 && idCount <= MAX_TRACK_REFERENCE_COUNT, Status::ERROR_INVALID_DATA,
        "Invalid id count in tref atom");

    auto idData = std::make_unique<uint8_t[]>(idCount * sizeof(uint32_t));
    ret = dataReader_->ReadUintData(ctx->dataOffset + sizeof(subAtomData), idData.get(), idCount * sizeof(uint32_t));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tref atom id data failed");

    std::vector<int32_t> referencedTrackIds;
    std::ostringstream referencedIdsStr;
    for (uint32_t i = 0; i < idCount; ++i) {
        int32_t trackId = static_cast<int32_t>(GetU32Value(&idData[i * sizeof(uint32_t)]));
        if (trackId > 0) {
            int32_t adjustedTrackId = trackId - 1;
            referencedTrackIds.emplace_back(adjustedTrackId);
            if (!referencedTrackIds.empty() && referencedTrackIds.size() > 1) {
                referencedIdsStr << ",";
            }
            referencedIdsStr << adjustedTrackId;
        }
    }

    if (!referencedTrackIds.empty()) {
        if (subAtomType == static_cast<uint32_t>(FourccType("cdsc"))) {
            int32_t referencedTrackId = referencedTrackIds[0];
            mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::TIMED_METADATA_SRC_TRACK>(referencedTrackId);
        }
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::REFERENCE_TRACK_IDS>(referencedTrackIds);
        MEDIA_LOG_D("ParseTref Referenced track IDs: " PUBLIC_LOG_S, referencedIdsStr.str().c_str());
    }

    ctx->offset += static_cast<int64_t>(currentAtom.size);
    return ret;
}

Status MPEG4AtomParser::ParseMdia(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_TWO, Status::ERROR_INVALID_DATA, "Invalid mdia depth");
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse mdia container failed");
    return ret;
}

int64_t GetMdhdDuration(const uint8_t* headerInfo, uint8_t version)
{
    uint32_t durationOffset = (version == 1) ? DURATION_OFFSET_V1 : DURATION_OFFSET_V0;
    if (version == 1) {
        uint64_t durationValue = GetU64Value(&headerInfo[durationOffset]);
        FALSE_RETURN_V_NOLOG(durationValue != UINT64_MAX && durationValue <= static_cast<uint64_t>(INT64_MAX), 0);
        return static_cast<int64_t>(durationValue);
    }
    uint32_t durationValue = GetU32Value(&headerInfo[durationOffset]);
    return durationValue == UINT32_MAX ? 0 : static_cast<int64_t>(durationValue);
}

uint16_t GetMdhdLanguageValue(const uint8_t* headerInfo, uint8_t version)
{
    constexpr int32_t languageOffsetVersionOne = 32;
    constexpr int32_t languageOffsetVersionZero = 20;
    int32_t languageOffset = (version == 1) ? languageOffsetVersionOne : languageOffsetVersionZero;
    return GetU16Value(&headerInfo[languageOffset]);
}

void MPEG4AtomParser::UpdateMdhdTimeOffset(uint32_t timeScale)
{
    int32_t fileTimeScale = movieTimeScale_;
    FALSE_RETURN_NOLOG(fileTimeScale > 0 && lastTrack_->sampleHelper != nullptr);
    if (!lastTrack_->editListEntries.empty()) {
        lastTrack_->sampleHelper->SetTimeOffset(0);
        return;
    }
    int64_t emptyDurationTrackTs = 0;
    if (lastTrack_->elstInitEmptyEdit > 0) {
        int64_t scaledEmptyDuration = 0;
        int64_t roundOffset = static_cast<int64_t>(fileTimeScale) - 1;
        if (__builtin_mul_overflow(lastTrack_->elstInitEmptyEdit, static_cast<int64_t>(timeScale),
            &scaledEmptyDuration) ||
            __builtin_add_overflow(scaledEmptyDuration, roundOffset, &scaledEmptyDuration)) {
            MEDIA_LOG_E("Convert empty edit duration overflow");
            return;
        }
        emptyDurationTrackTs = scaledEmptyDuration / static_cast<int64_t>(fileTimeScale);
    }
    lastTrack_->sampleHelper->SetTimeOffset(lastTrack_->elstShiftStartTime - emptyDurationTrackTs);
}

Status MPEG4AtomParser::ParseMdhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    constexpr uint64_t minMdhdSize = 24; // 最小mdhd atom大小
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= minMdhdSize, Status::ERROR_INVALID_DATA, "Invalid mdhd atom size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    uint8_t version = 0;
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, &version, sizeof(version));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read mdhd version failed");
    FALSE_RETURN_V_MSG_E(version == 0 || version == 1, Status::ERROR_INVALID_DATA, "Invalid mdhd version");
    constexpr uint64_t mdhdReadSizeV0 = 24;
    constexpr uint64_t mdhdReadSizeV1 = 34;
    uint64_t mdhdReadSize = (version == 1) ? mdhdReadSizeV1 : mdhdReadSizeV0;
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= mdhdReadSize, Status::ERROR_INVALID_DATA,
        "Invalid mdhd atom size for version");
    std::array<uint8_t, mdhdReadSizeV1> headerInfo = {};
    ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo.data(), mdhdReadSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read mdhd info failed");
    // skip some unused attributes
    uint32_t timeScaleOffset = (version == 1) ? OFFSET_V1 : OFFSET_V0;
    uint32_t timeScale = GetU32Value(&headerInfo.data()[timeScaleOffset]);
    if (timeScale <= 0) {
        MEDIA_LOG_E("Invalid timescale");
        timeScale = 1;  // set default timescale to 1
    }
    lastTrack_->duration = GetMdhdDuration(headerInfo.data(), version);
    lastTrack_->timeScale = static_cast<int32_t>(timeScale);
    std::string langString = ConvertLanguageCode(GetMdhdLanguageValue(headerInfo.data(), version));
    if (langString.size() > 0) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_LANGUAGE>(langString);
    }

    UpdateMdhdTimeOffset(timeScale);
    return ret;
}

bool MPEG4AtomParser::UnderMetaPath(const std::vector<uint32_t>& path, int32_t depth)
{
    FALSE_RETURN_V_MSG_E(path.size() >= DIRECT_META_PATH_SIZE, false, "Invalid depth for meta path check");
    constexpr size_t pathTwo = 2;
    constexpr size_t pathThree = 3;
    constexpr size_t metaPathSize = 3;   // moov/udta/meta 结构的路径大小
    constexpr size_t hdlrPathSize = 4;   // 包含hdlr的最大路径大小

    // 情况1: moov/meta 直接结构
    FALSE_RETURN_V_NOLOG(path.size() < DIRECT_META_PATH_SIZE || path[0] != FourccType("moov") ||
        path[1] != FourccType("meta"), (depth == MPEG4_DEPTH_TWO || (depth == MPEG4_DEPTH_THREE &&
        path.size() >= metaPathSize && path[pathTwo] == FourccType("hdlr"))));

    // 情况2：moov/udta/meta 结构
    FALSE_RETURN_V_NOLOG(path.size() < metaPathSize || path[0] != FourccType("moov") ||
        path[1] != FourccType("udta") || path[pathTwo] != FourccType("meta"),
        (depth == MPEG4_DEPTH_THREE || (depth == MPEG4_DEPTH_FOUR &&
        path.size() >= hdlrPathSize && path[pathThree] == FourccType("hdlr"))));

    return false;
}

bool MPEG4AtomParser::UnderFileLevelMetaPath(const std::vector<uint32_t>& path, int32_t depth)
{
    return depth == MPEG4_DEPTH_ONE && path.size() == DIRECT_META_PATH_SIZE &&
        path[0] == FourccType("meta");
}

void MPEG4AtomParser::TrySetGltfOffset(ParseContext* ctx)
{
    FALSE_RETURN_NOLOG(ctx != nullptr);
    if (ctx->foundFileMetaHdlrGltf && ctx->fileMetaIdatPayloadOffset >= 0) {
        mediaInfo_.general.Set<Tag::GLTF_OFFSET>(ctx->fileMetaIdatPayloadOffset);
    }
}

Status MPEG4AtomParser::SetTrackTypeByHdlr(MPEG4Atom currentAtom, uint32_t handlerType, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "not in a valid track");
    switch (handlerType) {
        case FourccType("vide"):
        case FourccType("covr"): {
            lastTrack_->codecParms.trackType = VIDEO_TYPE;
            break;
        }
        case FourccType("soun"): {
            lastTrack_->codecParms.trackType = AUDIO_TYPE;
            break;
        }
        case FourccType("subp"):
        case FourccType("clcp"):
        case FourccType("text"): {
            lastTrack_->codecParms.trackType = SUBTITLE_TYPE;
            break;
        }
        case FourccType("auxv"): {
            lastTrack_->codecParms.trackType = AUXILIARY_TYPE;
            Status ret = ProcessAuxiliaryHandler(currentAtom, ctx);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Process auxv handler failed");
            break;
        }
        case FourccType("meta"): {
            Status ret = ProcessMetaHandler(currentAtom, ctx);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Process meta handler failed");
            break;
        }
        default: {
            MEDIA_LOG_W("Unsupported track: hdlr[%{public}s]", FourccToString(handlerType).c_str());
            return Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT;
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseHdlr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_HDLR_DATA_SIZE, Status::ERROR_INVALID_DATA, "Invalid size");
    // Skip version and flags (4 bytes), then read componentType and handlerType
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    uint8_t typeInfo[12];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, typeInfo, sizeof(typeInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read hdlr type info failed");
    uint32_t handlerType = GetU32Value(&typeInfo[8]);
    if (UnderFileLevelMetaPath(ctx->path, depth)) {
        ctx->foundFileMetaHdlrGltf = ctx->foundFileMetaHdlrGltf ||
                                    handlerType == static_cast<uint32_t>(FourccType("gltf"));
        TrySetGltfOffset(ctx);
        return Status::OK;
    }
    // If this is a metadata handler outside a valid track, mark it and return
    if (UnderMetaPath(ctx->path, depth)) {
        ctx->foundHdlrMdta = handlerType == static_cast<uint32_t>(FourccType("mdta")) ? true : false;
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ret = SetTrackTypeByHdlr(currentAtom, handlerType, ctx);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    lastTrack_->sampleHelper->trackType_ = lastTrack_->codecParms.trackType;
    SetTrackMediaType(mediaInfo_.tracks[lastTrack_->trackIndex], lastTrack_->sampleHelper->trackType_);
    return ret;
}

Status MPEG4AtomParser::ProcessMetaHandler(MPEG4Atom currentAtom, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    uint64_t titleSize = currentAtom.dataSize - MAX_TITLE_SIZE;
    FALSE_RETURN_V_MSG_W(titleSize > 0, Status::OK, "ProcessMetaHandler titleSize <= 0");
    FALSE_RETURN_V_MSG_E(titleSize <= std::min(static_cast<uint64_t>(INT_MAX), static_cast<uint64_t>(SIZE_MAX - 1)),
        Status::ERROR_INVALID_DATA, "Titlesize is invalid");
    std::vector titleBuffer(static_cast<size_t>(titleSize) + 1, '\0');
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + MAX_TITLE_SIZE,
        reinterpret_cast<uint8_t*>(titleBuffer.data()), titleSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read meta handler title failed");

    int32_t offset = (!isIsom_ && static_cast<uint8_t>(titleBuffer[0]) == titleSize - 1) ? 1 : 0;
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::TRACK_DESCRIPTION>(titleBuffer.data() + offset);
    constexpr uint32_t timedMetaDataLength = 14;

    if (titleSize >= static_cast<uint64_t>(offset + timedMetaDataLength) &&
        strncmp(titleBuffer.data() + offset, "timed_metadata", timedMetaDataLength) == 0) {
        lastTrack_->codecParms.trackType = TIMEDMETA_TYPE;
        lastTrack_->sampleHelper->trackType_ = TIMEDMETA_TYPE;
    }
    return Status::OK;
}

Status MPEG4AtomParser::ProcessAuxiliaryHandler(MPEG4Atom currentAtom, ParseContext *ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    uint64_t titleSize = currentAtom.dataSize - MAX_TITLE_SIZE;
    FALSE_RETURN_V_MSG_W(titleSize > 0, Status::OK, "ProcessAuxiliaryHandler titleSize <= 0");
    FALSE_RETURN_V_MSG_E(titleSize <= std::min(static_cast<uint64_t>(INT_MAX), static_cast<uint64_t>(SIZE_MAX - 1)),
        Status::ERROR_INVALID_DATA, "Titlesize is invalid");
    std::vector titleBuffer(static_cast<size_t>(titleSize) + 1, '\0');
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + MAX_TITLE_SIZE,
        reinterpret_cast<uint8_t *>(titleBuffer.data()), titleSize);

    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read auxiliary handler title failed");
    int32_t offset = (!isIsom_ && static_cast<uint8_t>(titleBuffer[0]) == titleSize - 1) ? 1 : 0;
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::TRACK_DESCRIPTION>(titleBuffer.data() + offset);
    return Status::OK;
}

Status MPEG4AtomParser::ParseMinf(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_THREE, Status::ERROR_INVALID_DATA, "Invalid minf depth");
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse minf container failed");
    return ret;
}

Status MPEG4AtomParser::ParseStbl(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_FOUR, Status::ERROR_INVALID_DATA, "Invalid stbl depth");

    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse stbl container failed");
    return ret;
}

void MPEG4AtomParser::InferTrackTypeFromMime(const std::string& mimeType)
{
    FALSE_RETURN_MSG(IsLastTrackValid(), "Invalid current track");
    if (lastTrack_->codecParms.trackType != INVALID_TYPE) {
        return;
    }
    if (IsVideoMimeType(mimeType) || IsImageMimeType(mimeType)) {
        lastTrack_->codecParms.trackType = VIDEO_TYPE;
        lastTrack_->sampleHelper->trackType_ = VIDEO_TYPE;
    } else if (IsAudioMimeType(mimeType)) {
        lastTrack_->codecParms.trackType = AUDIO_TYPE;
        lastTrack_->sampleHelper->trackType_ = AUDIO_TYPE;
    } else {
        return;
    }
    SetTrackMediaType(mediaInfo_.tracks[lastTrack_->trackIndex], lastTrack_->sampleHelper->trackType_);
}

Status MPEG4AtomParser::ParseSampleDescriptionEntry(int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    int64_t atomOffset = ctx->offset;
    uint8_t sampleEntryHeader[8];
    Status ret = dataReader_->ReadUintData(atomOffset, sampleEntryHeader, sizeof(sampleEntryHeader));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sample entry header failed");
    uint32_t atomSize = GetU32Value(&sampleEntryHeader[0]);
    FALSE_RETURN_V_MSG_E(atomSize >= sizeof(sampleEntryHeader), Status::ERROR_INVALID_DATA,
                         "Invalid sample entry size");
    MPEG4Atom atom {
        .type = static_cast<int32_t>(GetU32Value(&sampleEntryHeader[4])), .size = atomSize,
        .dataSize = atomSize - sizeof(sampleEntryHeader)
    };
    ctx->dataOffset = atomOffset + sizeof(sampleEntryHeader);
    std::string mimeType = GetMimeType(atom.type);
    FALSE_RETURN_V_MSG_E(IsSupportCodec(atom.type, lastTrack_->codecParms.trackType, mimeType) ||
        atom.type == FourccType("encv") || atom.type == FourccType("enca"),
        Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT,
        "Unsupported track: fourcc[%{public}s] track[%{public}d] mimeType[%{public}s]",
        FourccToString(atom.type).c_str(), lastTrack_->codecParms.trackType, mimeType.c_str());
    InferTrackTypeFromMime(mimeType);
    if (lastTrack_->codecParms.trackType == VIDEO_TYPE && IsImageMimeType(mimeType)) {
        ctx->offset += atom.size;
        return Status::OK;
    }
    if (NeedParseVideoInfo(mimeType, lastTrack_->codecParms.trackType) ||
        atom.type == FourccType("encv")) {
        return ParseVideoSampleEntry(atom, depth + 1, ctx);
    }
    if (NeedParseAudioInfo(mimeType, lastTrack_->codecParms.trackType) ||
        atom.type == FourccType("enca")) {
        return ParseAudioSampleEntry(atom, depth + 1, ctx);
    }
    if (lastTrack_->codecParms.trackType == SUBTITLE_TYPE) {
        return ParseSubtitleSampleEntry(atom, depth + 1, ctx);
    }
    if (lastTrack_->codecParms.trackType == TIMEDMETA_TYPE) {
        return ParseMebx(atom, depth + 1, ctx);
    }
    MEDIA_LOG_W("Unsupported track: type[%{public}d]", lastTrack_->codecParms.trackType);
    return Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT;
}

Status MPEG4AtomParser::ParseStsd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE, Status::ERROR_INVALID_DATA, "Invalid atom size");

    uint8_t stsdData[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, stsdData, sizeof(stsdData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read stsd entry count failed");
    uint32_t entryCount = GetU32Value(&stsdData[4]);
    FALSE_RETURN_V_MSG_E(entryCount > 0 && entryCount <= 0x0400, Status::ERROR_INVALID_DATA, "Invalid STSD entries");
    int64_t endOffset = ctx->dataOffset + static_cast<int64_t>(currentAtom.dataSize);
    ctx->offset = ctx->dataOffset + sizeof(stsdData); // 跳过stsd头部信息
    // 读取sample description entries
    for (uint32_t i = 0; i < entryCount; ++i) {
        ret = ParseSampleDescriptionEntry(depth, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse stsd container failed");
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA, "Invalid stsd size");
    if (lastTrack_->sampleHelper != nullptr &&
        NeedParseAudioInfo(lastTrack_->sampleHelper->mimeType_, lastTrack_->codecParms.trackType)) {
        ret = SetAudioAttribute();
    }
    return ret;
}

void MPEG4AtomParser::SetMime(std::string mimeType)
{
    lastTrack_->sampleHelper->mimeType_ = mimeType;
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimeType);
}

// 该解析函数专门用于处理视频SampleEntry atom，如avc1 hvc1 vvc1 enca
Status MPEG4AtomParser::ParseVideoSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_VIDEO_SAMPLE_ENTRY_SIZE, Status::ERROR_INVALID_DATA,
        "Invalid video codec atom size");
    /* Video SampleEntry的数据大小至少为78字节
     * pre_defined(2) + reserved(2) + pre_defined(12) + width(2) + height(2) +
     * horizresolution(4) + vertresolution(4) + reserved(4) + frame_count(2) +
     * compressorname(32) + depth(2) + pre_defined(2)
    */
    uint8_t sampleEntryInfo[78];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, sampleEntryInfo, sizeof(sampleEntryInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read Video Sample Entry info failed");

    uint16_t width = GetU16Value(&sampleEntryInfo[24]);
    uint16_t height = GetU16Value(&sampleEntryInfo[26]);
    MEDIA_LOG_D("width: " PUBLIC_LOG_U16 ", height: " PUBLIC_LOG_U16, width, height);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_WIDTH>(static_cast<int32_t>(width));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_HEIGHT>(static_cast<int32_t>(height));
    lastTrack_->stsdWidth = width;
    lastTrack_->stsdHeight = height;

    if (currentAtom.type != FourccType("encv")) {
        std::string mimeType = GetMimeType(currentAtom.type);
        SetMime(mimeType);
    }

    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("Atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    ctx->offset = ctx->dataOffset + sizeof(sampleEntryInfo);
    while (ctx->offset < endOffset) {
        ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse Video Sample Entry failed");
    }

    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA, "Invalid Video Sample Entry size");
    return ret;
}

Status MPEG4AtomParser::ParseSubtitleSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    if (currentAtom.type != FourccType("wvtt")) {
        ctx->offset += static_cast<int64_t>(currentAtom.size);
        return Status::OK;
    }
    SetMime(GetMimeType(currentAtom.type));

    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("WebVTT sample entry size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    constexpr uint64_t webVttSampleEntryDataSize = 8;
    if (currentAtom.dataSize < webVttSampleEntryDataSize) {
        ctx->offset = endOffset;
        return Status::OK;
    }

    // The wvtt sample entry has reserved/data-reference fields before vttC.
    ctx->offset = ctx->dataOffset + static_cast<int64_t>(webVttSampleEntryDataSize);
    while (ctx->offset < endOffset) {
        Status ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse WebVTT sample entry child failed");
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA,
        "Invalid WebVTT sample entry size");
    return Status::OK;
}

Status MPEG4AtomParser::ParseMebx(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_TIMED_METADATA_SAMPLE_ENTRY_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid mebx atom size");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "mebx"), Status::ERROR_INVALID_DATA,
        "Invalid mebx data size");
    constexpr uint64_t skipSize = 8;
    // 设置MIME类型
    std::string mimeType = GetMimeType(currentAtom.type);
    SetMime(mimeType);

    // 读取mebx数据
    auto mebxData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(mebxData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate memory for mebx data");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, mebxData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read mebx data failed");

    uint64_t readOffset = 0;
    // 跳过前8字节的填充数据
    readOffset += skipSize;
    // 读取size_keys
    uint32_t sizeKeys = GetU32Value(&mebxData[readOffset]);
    // 跳过'keys'标识
    readOffset += skipSize;
    uint64_t readSize = skipSize; // read_counts = 8 (已读取size_keys和keys fourcc)
    std::string timedMetadataKey;

    uint64_t skipOffset = 16;
    uint64_t skipKeydOffset = 12;
    while (readSize < sizeKeys && readOffset + skipOffset <= currentAtom.dataSize) {
        // 读取atom_size
        uint32_t atomSize = GetU32Value(&mebxData[readOffset]);
        // 跳过local key id
        readOffset += skipSize;
        // 读取keyd_size
        uint32_t keydSize = GetU32Value(&mebxData[readOffset]);
        // 跳过'keyd' 'mdta'
        readOffset += skipKeydOffset;

        FALSE_RETURN_V_MSG_E(keydSize > skipKeydOffset, Status::ERROR_INVALID_DATA, "Invalid keyd size");
        uint32_t keyValueSize = keydSize - skipKeydOffset;
        if (readOffset + keyValueSize <= currentAtom.dataSize && keyValueSize > 0) {
            std::string keyValue(reinterpret_cast<const char*>(&mebxData[readOffset]), keyValueSize);
            if (!keyValue.empty()) {
                timedMetadataKey = keyValue;
            }
            readOffset += keyValueSize;
        } else {
            MEDIA_LOG_E("Invalid key value size: " PUBLIC_LOG_U32, keyValueSize);
            break;
        }
        readSize += atomSize;
    }
    if (!timedMetadataKey.empty()) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::TIMED_METADATA_KEY>(timedMetadataKey);
    }
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    MEDIA_LOG_D("Mebx parsing completed. Key: " PUBLIC_LOG_S, timedMetadataKey.c_str());
    return Status::OK;
}

// 该解析函数专门用于处理CodecConfig atom，如avcC hvcC vvcC
Status MPEG4AtomParser::ParseCodecConfig(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize <= MAX_EXTRA_DATA_SIZE, Status::ERROR_INVALID_DATA,
        "CodecConfig size too large");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    FALSE_RETURN_V_MSG_D(currentAtom.dataSize > 0 || currentAtom.type == FourccType("vttC"), Status::OK,
        "Ignore empty atom");
    // 检查是否已经有 codec config 数据
    int64_t dataOffset = ctx->dataOffset;
    uint64_t dataSize = currentAtom.dataSize;
    if (currentAtom.type == FourccType("vvcC")) {
        FALSE_RETURN_V_MSG_E(dataSize > FULLBOX_PREFIX_SIZE, Status::ERROR_INVALID_DATA,
            "vvcC data too small to contain FullBox prefix");
        dataOffset += FULLBOX_PREFIX_SIZE;
        dataSize -= FULLBOX_PREFIX_SIZE;
    } else if (currentAtom.type == FourccType("vttC")) {
        // FFmpeg exposes the complete vttC box as subtitle extradata.
        FALSE_RETURN_V_MSG_E(currentAtom.size <= static_cast<uint64_t>(INT64_MAX) && ctx->offset >= 0 &&
            static_cast<uint64_t>(ctx->offset) >= currentAtom.size,
            Status::ERROR_INVALID_DATA, "Invalid vttC atom offsets");
        const int64_t atomStart = ctx->offset - static_cast<int64_t>(currentAtom.size);
        FALSE_RETURN_V_MSG_E(ctx->dataOffset >= atomStart, Status::ERROR_INVALID_DATA,
            "Invalid vttC data offset");
        const int64_t atomHeaderSize = ctx->dataOffset - atomStart;
        FALSE_RETURN_V_MSG_E(atomHeaderSize >= static_cast<int64_t>(DEFAULT_HEAD_SIZE) &&
            static_cast<uint64_t>(atomHeaderSize) <= currentAtom.size, Status::ERROR_INVALID_DATA,
            "Invalid vttC atom header size");
        dataOffset -= atomHeaderSize;
        dataSize = static_cast<uint64_t>(currentAtom.size);
    }
    lastTrack_->codecParms.hasValidExtradata = false;
    lastTrack_->codecParms.annexbExtradataSize = 0;
    if (lastTrack_->codecParms.data != nullptr) {
        MEDIA_LOG_W("CodecConfig already exists, replacing with new data. Old size: " PUBLIC_LOG_D64 ", New size: "
            PUBLIC_LOG_D64, lastTrack_->codecParms.extradataSize, dataSize);
        lastTrack_->codecParms.data.reset();
        lastTrack_->codecParms.extradataSize = 0;
    }

    auto configData = std::make_unique<uint8_t[]>(dataSize);
    FALSE_RETURN_V_MSG_E(configData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate CodecConfig extradata");
    Status ret = dataReader_->ReadUintData(dataOffset, configData.get(), dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_NOT_ENOUGH_DATA, "Read CodecConfig data failed");

    lastTrack_->codecParms.data = std::move(configData);
    lastTrack_->codecParms.extradataSize = static_cast<int64_t>(dataSize);
    MEDIA_LOG_D("CodecConfig extradata size: " PUBLIC_LOG_D64, lastTrack_->codecParms.extradataSize);
    return ret;
}

// 该解析函数专门用于处理音频SampleEntry atom，如mp4a enca
Status MPEG4AtomParser::ParseAudioSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");

    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("Atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    Status ret = Status::OK;
    // 音频配置信息至少需要28字节的数据
    if (currentAtom.dataSize >= MIN_AUDIO_SAMPLE_ENTRY_SIZE) {
        ret = ParseAudioAttributes(currentAtom, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse audio attributes failed");
    } else {
        MEDIA_LOG_W("Audio sample entry too small (" PUBLIC_LOG_U64 " bytes), skipping parsing", currentAtom.dataSize);
        ctx->offset = endOffset;
    }

    // 递归解析子atom
    while (ctx->offset < endOffset) {
        ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse Audio Sample Entry child failed");
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA, "Invalid Audio Sample Entry size");

    return ret;
}

Status MPEG4AtomParser::ParseAudioVersionInfo(MPEG4Atom currentAtom, AudioVersionInfoContext& context)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ParseContext* ctx = context.ctx;
    if (isIsom_ && (ctx->compatibleBrands.count(FourccType("qt  "))) == 0 && context.version == 0) {
        return Status::OK;
    }
    constexpr int32_t versionOneSkipSize = 16;
    constexpr int32_t versionTwoSkipSize = 36;
    constexpr uint16_t sampleSizeOffset = 20;
    constexpr uint16_t versionTwo = 2;
    size_t entryInfoSize = (context.version == 1) ? versionOneSkipSize : versionTwoSkipSize;
    auto entryInfo = std::make_unique<uint8_t[]>(entryInfoSize);
    Status ret = dataReader_->ReadUintData(ctx->offset, entryInfo.get(), entryInfoSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read entry info failed");
    if (context.version == 1) {
        context.samplePerFrame = GetU32Value(&entryInfo[0]);
        ctx->offset += versionOneSkipSize;
        return Status::OK;
    }
    if (context.version != versionTwo) {
        return Status::OK;
    }
    context.sampleRate = static_cast<uint32_t>(GetDoubleValue(&entryInfo[0x04]));
    context.channelCount = static_cast<uint16_t>(GetU32Value(&entryInfo[0x0C]));
    context.sampleSize = static_cast<uint16_t>(GetU32Value(&entryInfo[sampleSizeOffset]));
    context.samplePerFrame = GetU32Value(&entryInfo[0x20]);
    ctx->offset += versionTwoSkipSize;
    return Status::OK;
}

Status MPEG4AtomParser::ParseAudioAttributes(MPEG4Atom currentAtom, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    /* Audio SampleEntry的数据大小 reserved(8) + version (2) + revision_level(2)
     * + vendor(4) + channelcount(2) + samplesize(2) + pre_defined(2) + reserved2(2) + samplerate(4)
    */
    uint8_t sampleEntryInfo[28];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, sampleEntryInfo, sizeof(sampleEntryInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read Audio Sample Entry info failed");
    // 读取当前sample entry的信息
    uint16_t version = GetU16Value(&sampleEntryInfo[0x08]);
    uint16_t channelCount = GetU16Value(&sampleEntryInfo[0x10]);
    uint16_t sampleSize = GetU16Value(&sampleEntryInfo[0x12]); // bits_per_coded_sample
    lastTrack_->audioSampleEntryChannelCount = channelCount;
    lastTrack_->audioSampleEntryBitsPerCodedSample = sampleSize;
    //skip unneed distribute
    uint32_t sampleRate = GetU32Value(&sampleEntryInfo[0x18]) / CONVERT_SCALE;
    ctx->offset = ctx->dataOffset + sizeof(sampleEntryInfo);
    uint32_t samplePerFrame = 0;
    AudioVersionInfoContext audioVersionInfo {ctx, version, channelCount, sampleSize, sampleRate, samplePerFrame};
    ret = ParseAudioVersionInfo(currentAtom, audioVersionInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse audio version info failed");
    if (currentAtom.type != FourccType("enca")) {
        std::string mimeType = GetMimeType(currentAtom.type);
        SetMime(mimeType);
    }

    int32_t finalSampleSize = static_cast<int32_t>(sampleSize);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(channelCount));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(sampleRate));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(finalSampleSize);
    if (currentAtom.type == FourccType("apac")) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::INVALID_WIDTH);
    } else if (samplePerFrame) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(
            static_cast<int32_t>(samplePerFrame));
    }
    if (currentAtom.type == FourccType("av3a") || currentAtom.type == FourccType("dca3")) {
        FALSE_LOG_MSG_W(ParseDca3Entry(currentAtom, ctx) == Status::OK, "Parse AV3A entry failed");
    }
    return ret;
}

void MPEG4AtomParser::SetAmrAudioAttribute(std::string mime)
{
    if (mime == MimeType::AUDIO_AMR_NB) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    } else {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    }
}

void MPEG4AtomParser::SetMpegAudioAttribute()
{
    int32_t channelCount = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    if (mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount)) {
        layout = channelCount == 1 ? AudioChannelLayout::MONO : AudioChannelLayout::STEREO;
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
    }
}

Status MPEG4AtomParser::SetAudioAttribute()
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    int32_t sampleRate = 0;
    int32_t timescale = lastTrack_->timeScale;
    mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    if (sampleRate == 0 && timescale > 0) {
        sampleRate = timescale;
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    }

    std::string mime = lastTrack_->sampleHelper->mimeType_;
    if (mime == MimeType::AUDIO_AAC) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_AAC_IS_ADTS>(1);
    }
    Status ret = Status::OK;
    int32_t bitsPerRawSample = 0;
    if (!mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(bitsPerRawSample)) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(bitsPerRawSample);
    }

    if (mime == MimeType::AUDIO_AMR_NB || mime == MimeType::AUDIO_AMR_WB) {
        SetAmrAudioAttribute(mime);
    } else if (mime == MimeType::AUDIO_MPEG) {
        SetMpegAudioAttribute();
    } else if (mime == MimeType::AUDIO_AAC) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
    }
    return ret;
}

bool ReadDescriptor(const uint8_t* data, uint64_t dataSize, uint64_t& offset, uint8_t& tag, uint32_t& size)
{
    FALSE_RETURN_V_MSG_E(offset < dataSize, false, "Read descriptor failed");
    tag = data[offset++];
    // 读取可变长度大小字段
    size = 0;
    uint8_t count = 0;
    uint8_t temp;
    const uint8_t maxCount = 4; // 最大可变长度大小字段为4字节
    do {
        FALSE_RETURN_V_NOLOG(offset < dataSize && count < maxCount, false);
        temp = data[offset++];
        size = (size << 0x07) | (temp & 0x7F);
        count++;
    } while (temp & 0x80);

    return true;
}

Status MPEG4AtomParser::ParseEsds(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "esds"), Status::ERROR_INVALID_DATA,
        "Invalid esds data size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    auto esdsData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, esdsData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read esds data failed");
    // 跳过 version + flags
    uint64_t offset = 4;

    // 跳过 ES Descriptor
    uint8_t tag;
    uint32_t size;
    FALSE_RETURN_V_MSG_E(ReadDescriptor(esdsData.get(), currentAtom.dataSize, offset, tag, size),
        Status::ERROR_INVALID_DATA, "Failed to read ES descriptor");

    const uint64_t skipOffsetMax = 3; // ES_ID + flags
    const uint64_t skipOffsetMin = 2;
    offset += (tag == MPEG4_ES_DESCR_TAG) ? skipOffsetMax : skipOffsetMin;

    // 读取 Decoder Config Descriptor
    FALSE_RETURN_V_MSG_E(ReadDescriptor(esdsData.get(), currentAtom.dataSize, offset, tag, size),
        Status::ERROR_INVALID_DATA, "Failed to read decoder config descriptor");
    FALSE_RETURN_V_MSG_E(tag == MPEG4_DEC_CONFIG_DESCR_TAG, Status::ERROR_INVALID_DATA,
        "Expected decoder config descriptor, got tag: " PUBLIC_LOG_U8, tag);

    // 解析 Decoder Config Descriptor
    ret = ParseDecoderConfigDescriptor(esdsData.get(), currentAtom.dataSize, offset);
    FALSE_RETURN_V_NOLOG(ret != Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, ret);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_INVALID_DATA, "Failed to parse decoder config descriptor");

    return Status::OK;
}

bool MPEG4AtomParser::IsSupportEsds()
{
    // mp4a vorbis
    if (lastTrack_ != nullptr && lastTrack_->codecParms.extradataSize >= VORBIS_EXTRADATA_SIZE &&
        lastTrack_->codecParms.data[OFFSET_4] == 0x76 && lastTrack_->codecParms.data[OFFSET_5] == 0x6F &&
        lastTrack_->codecParms.data[OFFSET_6] == 0x72 && lastTrack_->codecParms.data[OFFSET_7] == 0x62 &&
        lastTrack_->codecParms.data[OFFSET_8] == 0x69 && lastTrack_->codecParms.data[OFFSET_9] == 0x73) {
        return false;
    }
    return true;
}

bool MPEG4AtomParser::IsSupportObjectTypeId(uint8_t objectTypeId)
{
    switch (objectTypeId) {
        case 0xA9: // DTS Core AUdio
        case 0xAA: // DTS-HD Master Audio
        case 0xAC: // DTS-HD High Resolution Audio
            return false;
        default:
            return true;
    }
}

Status MPEG4AtomParser::ParseDecoderConfigDescriptor(const uint8_t* data, uint64_t dataSize, uint64_t& offset)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(offset + ESDS_READ_SIZE <= dataSize, Status::ERROR_INVALID_DATA, "Parse decoderConfig failed");

    uint8_t objectTypeId = data[offset++];         // Object Type ID
    offset += sizeof(uint64_t); // 跳过 streamType(1字节) buffer size (3字节) 和 max bitrate (4字节)

    std::string mimeType = GetMimeTypeByObjectType(objectTypeId);
    if (!mimeType.empty()) {
        SetMime(mimeType);
    }
    FALSE_RETURN_V_MSG_E(
        IsSupportObjectTypeId(objectTypeId), Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, "Unsupported track");
    uint32_t avgBitrate = GetU32Value(&data[offset]);
    offset += sizeof(uint32_t);

    if (lastTrack_ && avgBitrate > 0) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(avgBitrate));
    }
    // 读取解码器特定信息描述符
    uint8_t tag;
    uint32_t size;
    FALSE_RETURN_V_MSG_E(ReadDescriptor(data, dataSize, offset, tag, size), Status::ERROR_INVALID_DATA, "No info");

    if (tag == MPEG4_DEC_SPECIFIC_DESCR_TAG) {
        // 特殊情况：MPEG-1/2 Audio 不需要 decoder specific info
        FALSE_RETURN_V_MSG_D(objectTypeId != 0x69 && objectTypeId != 0x6B, Status::OK,
            "MPEG-1/2 Audio detected, no decoder specific info needed");
        FALSE_RETURN_V_MSG_E(size > 0 && size <= MAX_EXTRA_DATA_SIZE, Status::ERROR_INVALID_DATA,
            "Wrong descriptor size");
        FALSE_RETURN_V_MSG_E(offset + size <= dataSize, Status::ERROR_INVALID_DATA, "Wrong descriptor size");

        // 提取 decoder specific info
        const uint8_t* specificInfo = &data[offset];
        if (specificInfo && size > 0) {
            lastTrack_->codecParms.data = std::make_unique<uint8_t[]>(size);
            FALSE_RETURN_V_MSG_E(lastTrack_->codecParms.data, Status::ERROR_INVALID_DATA, "Alloc failed");

            auto ret = memcpy_s(lastTrack_->codecParms.data.get(), size, specificInfo, size);
            FALSE_RETURN_V_MSG_E(ret == EOK, Status::ERROR_INVALID_DATA, "Copy failed");
            lastTrack_->codecParms.extradataSize = static_cast<int64_t>(size);
            FALSE_RETURN_V_MSG_E(IsSupportEsds(), Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, "Unsupported track");
        }
        if (mimeType == MimeType::AUDIO_AAC) {
            return ParseAACSpecificConfig(specificInfo, size) ? Status::OK : Status::ERROR_INVALID_DATA;
        }
    }
    return Status::OK;
}

bool MPEG4AtomParser::ParseAACConfig(const uint8_t* data, uint32_t size, uint32_t& sampleRate, uint32_t& channels,
    AudioChannelLayout &layout)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), false, "Invalid current track");
    FALSE_RETURN_V_MSG_E(data != nullptr, false, "Invalid AAC config data");
    constexpr uint8_t channelConfigSize = 4;
    constexpr uint8_t baseConfigSize = 13;
    DemuxerBitReader bitReader(data, size);
    FALSE_RETURN_V(bitReader.HasBits(baseConfigSize), false);
    uint8_t audioObjectType = 0;
    ReadAACObjectType(bitReader, audioObjectType);
    uint8_t samplingFreqIndex = 0;
    sampleRate = 0;
    ReadAACSampleRate(bitReader, samplingFreqIndex, sampleRate);
    uint8_t channelConfig = static_cast<uint8_t>(ReadBitsWidePadded(bitReader, channelConfigSize));
    static const uint8_t aacAudioChannels[14] = {0, 1, 2, 3, 4, 5, 6, 8, 0, 0, 0, 7, 8, 24};
    if (channelConfig >= sizeof(aacAudioChannels) / sizeof(aacAudioChannels[0])) {
        MEDIA_LOG_E("Invalid AAC channel configuration: " PUBLIC_LOG_U8, channelConfig);
        return false;
    }
    channels = aacAudioChannels[channelConfig];
    layout = AAC_CHANNEL_LAYOUT_TABLE[channelConfig];

    const bool isImplicitPs = audioObjectType == AOT_PS &&
        !((bitReader.ShowBits(0x03) & 0x03) && !(ShowBitsWide(bitReader, 0x09) & 0x3F));
    if (audioObjectType == AOT_SBR || isImplicitPs) {
        uint8_t extensionSamplingFreqIndex = 0;
        uint32_t extensionSampleRate = 0;
        ReadAACSampleRate(bitReader, extensionSamplingFreqIndex, extensionSampleRate);
        ReadAACObjectType(bitReader, audioObjectType);
        if (extensionSampleRate > 0) {
            sampleRate = extensionSampleRate;
        }
    }
    const uint32_t frameLengthFlag = bitReader.HasBits(1) ? bitReader.ReadBits(1) : 0;
    uint32_t samplePerFrame = frameLengthFlag ? 960 : 1024;
    if (audioObjectType == AAC_ER_LD || (audioObjectType == AAC_ER_ELD)) {
        samplePerFrame >>= 1;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(samplePerFrame);
    MEDIA_LOG_D("AAC Config: objType=" PUBLIC_LOG_U8 ", samplingIdx=" PUBLIC_LOG_U8 ", channels="
        PUBLIC_LOG_U8, audioObjectType, samplingFreqIndex, channelConfig);
    return true;
}

bool MPEG4AtomParser::ParseAACSpecificConfig(const uint8_t* data, uint32_t size)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), false, "Invalid current track");
    constexpr uint32_t minConfigSize = 2;
    FALSE_RETURN_V_MSG_E(data && size >= minConfigSize, false, "Invalid AAC config data");
    uint32_t sampleRate = 44100;
    uint32_t channels = 2;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    if (ParseAACConfig(data, size, sampleRate, channels, layout)) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(channels));
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(sampleRate));
        if (channels > 0 && layout == AudioChannelLayout::UNKNOWN) {
            int32_t channelCount = 0;
            auto audioParser = std::make_shared<MPEG4AudioParser>();
            FALSE_RETURN_V_MSG_E(audioParser != nullptr, false, "Failed create audioParser");
            FALSE_RETURN_V_MSG_E(
                mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount),
                false, "Channel count missed");
            layout = audioParser->GetDefaultChannelLayout(channelCount);
        }
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNELS>(static_cast<int32_t>(channels));
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
        return true;
    }

    return false;
}

Status MPEG4AtomParser::ParseWave(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse moov atom failed");
    return ret;
}

Status MPEG4AtomParser::ParseBtrt(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    constexpr uint64_t btrtReadOffset = 4;
    constexpr uint64_t btrtReadSize = 8;
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= btrtReadOffset + btrtReadSize,
        Status::ERROR_INVALID_DATA, "Invalid btrt atom size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    uint8_t btrtInfo[btrtReadSize];
    // Skip version and flags (4 bytes)
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + btrtReadOffset, btrtInfo, sizeof(btrtInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read btrt info failed");
    uint32_t avgBitrate = GetU32Value(&btrtInfo[4]);
    MEDIA_LOG_D("avgBitrate=" PUBLIC_LOG_U32, avgBitrate);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(avgBitrate));
    return ret;
}

Status MPEG4AtomParser::ParsePasp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE, Status::ERROR_INVALID_DATA, "Invalid pasp size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    uint8_t paspInfo[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, paspInfo, sizeof(paspInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pasp info failed");
    uint32_t hSpacing = GetU32Value(&paspInfo[0]);
    uint32_t vSpacing = GetU32Value(&paspInfo[4]);
    if (vSpacing != 0) {
        lastTrack_->hSpacing = hSpacing;
        lastTrack_->vSpacing = vSpacing;
        lastTrack_->hasPasp = true;
        MEDIA_LOG_D("Parse pasp: hSpacing=%{public}u, vSpacing=%{public}u", hSpacing, vSpacing);
    } else {
        MEDIA_LOG_E("Invalid vSpacing=%{public}u", vSpacing);
    }
    return ret;
}

static Status ReadDca3ConfigData(const std::shared_ptr<DemuxerDataReader>& dataReader, int64_t dataOffset,
    std::unique_ptr<uint8_t[]>& dca3Data, uint64_t dataSize)
{
    FALSE_RETURN_V_NOLOG(dataSize != 0, Status::OK);
    FALSE_RETURN_V_MSG_E(dataSize >= AV3A_DCA3_BOX_MIN_SIZE && dataSize <= AV3A_DCA3_BOX_MAX_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid av3a config size");
    dca3Data = std::make_unique<uint8_t[]>(dataSize);
    FALSE_RETURN_V_MSG_E(dca3Data != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate dca3 buffer");
    Status ret = dataReader->ReadUintData(dataOffset, dca3Data.get(), dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read av3a config data failed");
    return Status::OK;
}

Status MPEG4AtomParser::ReadDca3Payload(MPEG4Atom currentAtom, ParseContext* ctx,
                                        std::unique_ptr<uint8_t[]>& dca3Data, uint64_t& dataSize)
{
    int64_t dataOffset = ctx->dataOffset;
    dataSize = currentAtom.dataSize;
    if (currentAtom.type == FourccType("av3a")) {
        int64_t dataEndOffset = 0;
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(ctx->dataOffset, currentAtom.dataSize, &dataEndOffset),
            Status::ERROR_INVALID_DATA, "av3a atom size overflow");
        FALSE_RETURN_V_MSG_E(ctx->offset >= ctx->dataOffset && ctx->offset <= dataEndOffset,
            Status::ERROR_INVALID_DATA, "Invalid av3a data offset");
        if (ctx->offset == dataEndOffset) {
            dataSize = 0;
            return Status::OK;
        }
        uint8_t atomHeader[8];
        Status ret = dataReader_->ReadUintData(ctx->offset, atomHeader, sizeof(atomHeader));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read dca3 header failed");
        MPEG4Atom dca3Atom;
        dca3Atom.size = GetU32Value(&atomHeader[0]);
        dca3Atom.type = static_cast<int32_t>(GetU32Value(&atomHeader[UINT32_SIZE]));
        FALSE_RETURN_V_MSG_E(dca3Atom.type == FourccType("dca3"), Status::ERROR_INVALID_DATA,
            "Invalid av3a child atom");
        int64_t dca3EndOffset = 0;
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(ctx->offset, dca3Atom.size, &dca3EndOffset),
            Status::ERROR_INVALID_DATA, "dca3 atom size overflow");
        FALSE_RETURN_V_MSG_E(dca3Atom.size >= sizeof(atomHeader) && dca3EndOffset <= dataEndOffset,
            Status::ERROR_INVALID_DATA, "Invalid dca3 atom size");
        dca3Atom.dataSize = dca3Atom.size - sizeof(atomHeader);
        dataOffset = ctx->offset + sizeof(atomHeader);
        dataSize = dca3Atom.dataSize;
        ctx->offset = dataEndOffset;
    } else if (currentAtom.type == FourccType("dca3")) {
        int64_t dataEndOffset = 0;
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(ctx->dataOffset, currentAtom.dataSize, &dataEndOffset),
            Status::ERROR_INVALID_DATA, "dca3 atom size overflow");
        FALSE_RETURN_V_MSG_E(ctx->offset >= ctx->dataOffset && ctx->offset <= dataEndOffset,
            Status::ERROR_INVALID_DATA, "Invalid dca3 data offset");
        dataOffset = ctx->offset;
        dataSize = static_cast<uint64_t>(dataEndOffset - ctx->offset);
        ctx->offset = dataEndOffset;
    } else {
        ctx->offset += static_cast<int64_t>(currentAtom.size);
    }
    return ReadDca3ConfigData(dataReader_, dataOffset, dca3Data, dataSize);
}

Status MPEG4AtomParser::ParseDca3ContentType(DemuxerBitReader& bitReader, uint8_t contentType,
                                             Av3aConfigParams& params)
{
    switch (contentType) {
        case AV3A_CHANNEL_BASED_TYPE:
        case AV3A_CHANNEL_OBJECT_TYPE: {
            // 8为av3a channel config长度
            FALSE_RETURN_V_MSG_E(bitReader.HasBits(8), Status::ERROR_INVALID_DATA, "av3a channel config missing");
            uint8_t channelNumberIndex = static_cast<uint8_t>(ReadBitsWide(bitReader, 7));
            FALSE_RETURN_V_MSG_E(bitReader.SkipBits(1), Status::ERROR_INVALID_DATA,
                "Skip av3a reserved channel bit failed");

            uint8_t minChannelConfig = (contentType == AV3A_CHANNEL_OBJECT_TYPE) ?
                AV3A_CHANNEL_CONFIG_STEREO : AV3A_CHANNEL_CONFIG_MONO;
            FALSE_RETURN_V_MSG_E(channelNumberIndex >= minChannelConfig &&
                channelNumberIndex <= AV3A_CHANNEL_CONFIG_7_1_4,
                Status::ERROR_INVALID_DATA, "Unsupported av3a channel config");
            FALSE_RETURN_V_MSG_E(GetAv3aChannelConfig(channelNumberIndex,
                params.channelLayout, params.soundbedChannels),
                Status::ERROR_INVALID_DATA, "Invalid av3a channel config");

            if (contentType == AV3A_CHANNEL_OBJECT_TYPE) {
                // 8为av3a object count长度
                FALSE_RETURN_V_MSG_E(bitReader.HasBits(8), Status::ERROR_INVALID_DATA, "av3a object count missing");
                // 7为av3a object数量有效位
                params.objectNumber = static_cast<int32_t>(ReadBitsWide(bitReader, 7));
                FALSE_RETURN_V_MSG_E(bitReader.SkipBits(1),
                    Status::ERROR_INVALID_DATA, "Skip reserved object bit failed");
                FALSE_RETURN_V_MSG_E(params.objectNumber > 0, Status::ERROR_INVALID_DATA, "Invalid av3a object count");
            }
            break;
        }
        case AV3A_OBJECT_BASED_TYPE: {
            // 8为av3a object count长度
            FALSE_RETURN_V_MSG_E(bitReader.HasBits(8), Status::ERROR_INVALID_DATA, "av3a object count missing");
            // 7为av3a object数量有效位
            params.objectNumber = static_cast<int32_t>(ReadBitsWide(bitReader, 7));
            FALSE_RETURN_V_MSG_E(bitReader.SkipBits(1), Status::ERROR_INVALID_DATA, "Skip reserved object bit failed");
            FALSE_RETURN_V_MSG_E(params.objectNumber > 0, Status::ERROR_INVALID_DATA, "Invalid av3a object count");
            break;
        }
        case AV3A_AMBISONIC_TYPE: {
            // 4为av3a hoa order长度
            FALSE_RETURN_V_MSG_E(bitReader.HasBits(4), Status::ERROR_INVALID_DATA, "av3a hoa order missing");
            // 4为av3a hoa order有效位
            params.hoaOrder = static_cast<int32_t>(ReadBitsWide(bitReader, 4));
            FALSE_RETURN_V_MSG_E(params.hoaOrder >= AV3A_AMBISONIC_FIRST_ORDER &&
                params.hoaOrder <= AV3A_AMBISONIC_THIRD_ORDER, Status::ERROR_INVALID_DATA, "Invalid av3a hoa order");
            // 计算声道布局和声床声道数
            params.channelLayout = (params.hoaOrder == 1) ? AudioChannelLayout::HOA_ORDER1_ACN_SN3D :
                                    // params.hoaOrder为2对应HOA_ORDER2_ACN_SN3D布局
                                    (params.hoaOrder == 2) ? AudioChannelLayout::HOA_ORDER2_ACN_SN3D :
                                                            AudioChannelLayout::HOA_ORDER3_ACN_SN3D;
            params.soundbedChannels = (params.hoaOrder + 1) * (params.hoaOrder + 1);
            break;
        }
        default:
            return Status::ERROR_INVALID_DATA;
    }
    return Status::OK;
}

Status MPEG4AtomParser::UpdateDca3TrackMeta(DemuxerBitReader& bitReader, const Av3aConfigParams& params)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    // 18为av3a bitrate和resolution长度
    FALSE_RETURN_V_MSG_E(bitReader.HasBits(18), Status::ERROR_INVALID_DATA, "av3a bitrate or resolution missing");
    uint32_t bitrateKbps = ReadBitsWide(bitReader, 16);
    FALSE_RETURN_V_MSG_E(bitrateKbps > 0, Status::ERROR_INVALID_DATA, "Invalid av3a bitrate");

    uint8_t resolutionIndex = static_cast<uint8_t>(ReadBitsWide(bitReader, 2));
    FALSE_RETURN_V_MSG_E(resolutionIndex < AV3A_RESOLUTION_TABLE.size() &&
        AV3A_RESOLUTION_TABLE[resolutionIndex].sampleFormat != AudioSampleFormat::INVALID_WIDTH,
        Status::ERROR_INVALID_DATA, "Invalid av3a resolution");

    int32_t totalChannels = params.soundbedChannels + params.objectNumber;
    auto& trackMeta = mediaInfo_.tracks[lastTrack_->trackIndex];

    // 1000为av3a bitrate转换为bps
    trackMeta.Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(bitrateKbps) * 1000);
    trackMeta.Set<Tag::AUDIO_SAMPLE_FORMAT>(AV3A_RESOLUTION_TABLE[resolutionIndex].sampleFormat);
    trackMeta.Set<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(AV3A_RESOLUTION_TABLE[resolutionIndex].bitsPerSample);
    if (lastTrack_->audioSampleEntryBitsPerCodedSample > 0) {
        trackMeta.Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(
            static_cast<int32_t>(lastTrack_->audioSampleEntryBitsPerCodedSample));
    }
    trackMeta.Set<Tag::AUDIO_SAMPLE_PER_FRAME>(AV3A_AUDIO_FRAME_SIZE);
    trackMeta.Set<Tag::AUDIO_OBJECT_NUMBER>(params.objectNumber);
    trackMeta.Set<Tag::AUDIO_SOUNDBED_CHANNELS_NUMBER>(params.soundbedChannels);

    if (params.hoaOrder > 0) trackMeta.Set<Tag::AUDIO_HOA_ORDER>(params.hoaOrder);
    if (totalChannels > 0) {
        trackMeta.Set<Tag::AUDIO_CHANNEL_COUNT>(totalChannels);
        trackMeta.Set<Tag::AUDIO_OUTPUT_CHANNELS>(totalChannels);
    }
    trackMeta.Set<Tag::AUDIO_CHANNEL_LAYOUT>(params.channelLayout);
    trackMeta.Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(params.channelLayout);

    return Status::OK;
}

Status MPEG4AtomParser::ParseDca3Entry(MPEG4Atom currentAtom, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(ctx != nullptr, Status::ERROR_NULL_POINTER, "Parse context is null");
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");

    std::unique_ptr<uint8_t[]> dca3Data;
    uint64_t dataSize = 0;

    // 读取有效载荷
    Status ret = ReadDca3Payload(currentAtom, ctx, dca3Data, dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read av3a payload failed");

    if (dataSize == 0) {
        auto& trackMeta = mediaInfo_.tracks[lastTrack_->trackIndex];
        trackMeta.Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::UNKNOWN);
        trackMeta.Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::UNKNOWN);
        return Status::OK;
    }

    auto bitReader = std::make_shared<DemuxerBitReader>(dca3Data.get(), dataSize);
    FALSE_RETURN_V_MSG_E(bitReader != nullptr, Status::ERROR_NO_MEMORY, "Failed create bitReader");
    // 16为av3a config data长度
    FALSE_RETURN_V_MSG_E(bitReader->HasBits(16), Status::ERROR_INVALID_DATA, "av3a config data too short");

    // 解析基础头部信息
    uint8_t audioCodecId = bitReader->ReadBits(4);
    FALSE_RETURN_V_MSG_E(audioCodecId == AV3A_LOSSY_CODEC_ID, Status::ERROR_INVALID_DATA, "Unsupported codec id");
    uint8_t samplingFrequencyIndex = bitReader->ReadBits(4);
    uint8_t nnType = bitReader->ReadBits(3);
    FALSE_RETURN_V_MSG_E(nnType >= AV3A_BASELINE_NN_TYPE && nnType <= AV3A_LC_NN_TYPE,
        Status::ERROR_INVALID_DATA, "Unsupported av3a nn type");
    FALSE_RETURN_V_MSG_E(bitReader->SkipBits(1), Status::ERROR_INVALID_DATA, "Skip reserved bit failed");
    uint8_t contentType = bitReader->ReadBits(4);

    // 解析 ContentType 对应详情
    Av3aConfigParams params;
    ret = ParseDca3ContentType(*bitReader, contentType, params);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ParseDca3ContentType failed");

    // 更新元数据标签
    ret = UpdateDca3TrackMeta(*bitReader, params);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "UpdateDca3TrackMeta failed");

    MEDIA_LOG_I("MPEG4AtomParser::ParseDca3 out");
    MEDIA_LOG_I("Parse av3a config success, samplingIndex=" PUBLIC_LOG_U8 ", contentType=" PUBLIC_LOG_U8
        ", soundbed=" PUBLIC_LOG_D32 ", objects=" PUBLIC_LOG_D32 ", layout=" PUBLIC_LOG_U64,
        samplingFrequencyIndex, contentType, params.soundbedChannels, params.objectNumber,
        static_cast<uint64_t>(params.channelLayout));

    return Status::OK;
}

Status MPEG4AtomParser::ParseColorParams(int64_t offset, uint64_t dataSize, const std::string& colorType,
    ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(dataSize >= MIN_COLOR_PARAM_SIZE, Status::ERROR_INVALID_DATA, "Invalid color parameters size");

    uint8_t colorParams[8];  // 读取颜色参数 最多8字节：6字节基本参数 + 可能的range字节
    uint32_t readSize = std::min(static_cast<uint64_t>(sizeof(colorParams)), dataSize);
    Status ret = dataReader_->ReadUintData(offset, colorParams, readSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read color parameters failed");

    uint16_t colorPrimaries = GetU16Value(&colorParams[0]);
    uint16_t colorTrc = GetU16Value(&colorParams[2]);
    uint16_t colorMatrix = GetU16Value(&colorParams[4]);
    uint8_t colorRange = 0;

    constexpr uint8_t colorRangeOffset = 6;
    if (colorType == "nclx" && dataSize >= 0x07) { // nclx类型有额外的color range信息
        uint8_t nclxColorRange = (colorParams[colorRangeOffset] >> 0x07) & 1;  // 取最高位
        if (nclxColorRange) {
            colorRange = 1;
        }
    }
    if (lastTrack_->sampleHelper->mimeType_ == MimeType::VIDEO_HEVC) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_COLOR_RANGE>(static_cast<bool>(colorRange));
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_COLOR_PRIMARIES>(
            static_cast<ColorPrimary>(colorPrimaries));
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_COLOR_TRC>(
            static_cast<TransferCharacteristic>(colorTrc));
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(
            static_cast<MatrixCoefficient>(colorMatrix));
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseColr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_COLOR_SIZE, Status::ERROR_INVALID_DATA, "colr atom too small");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    uint8_t colorParamType[4];
    int32_t typeSize = sizeof(colorParamType);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, colorParamType, sizeof(colorParamType));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read color parameter type failed");
    std::string colorType(reinterpret_cast<char*>(colorParamType), sizeof(colorParamType));

    FALSE_RETURN_V_MSG_W(colorType == "nclx" || colorType == "nclc" || colorType == "prof", Status::OK,
        "Unsupported color parameter type: " PUBLIC_LOG_S, colorType.c_str());
    uint64_t dataSize = 0;
    FALSE_RETURN_V_NOLOG(currentAtom.dataSize > static_cast<uint64_t>(typeSize), Status::ERROR_INVALID_DATA);
    dataSize = currentAtom.dataSize - static_cast<uint64_t>(typeSize);
    if (colorType != "prof") {
        ret = ParseColorParams(ctx->dataOffset + typeSize, dataSize, colorType, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse color parameters failed");
    }
    return ret;
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseAclr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_ACLR_SIZE &&
        CheckBoxDataAllocSize(currentAtom.dataSize, "aclr"), Status::ERROR_INVALID_DATA,
        "aclr atom size too small");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    auto aclrData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, aclrData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read aclr data failed");
    uint8_t colorRange = aclrData[12]; // 第13字节是colorRange
    switch (colorRange) {
        case 0x01:
            colorRange = 0; // MPEG range (16-235)
            break;
        case 0x02:
            colorRange = 1; // JPEG/PC range (0-255)
            break;
        default:
            break;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_COLOR_RANGE>(static_cast<bool>(colorRange));
    return Status::OK;
}

Status MPEG4AtomParser::ParseGlbl(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "glbl"), Status::ERROR_INVALID_DATA,
        "glbl atom size too large");
    constexpr uint64_t minCheckSize = 10;
    // 检查是否嵌套fiel atom
    if (currentAtom.dataSize >= minCheckSize) {
        uint8_t header[8];
        constexpr uint32_t typeOffset = 4;
        Status ret = dataReader_->ReadUintData(ctx->dataOffset, header, sizeof(header));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read glbl header failed");
        uint32_t size = GetU32Value(&header[0]);
        uint32_t type = GetU32Value(&header[typeOffset]);
        if (type == static_cast<uint32_t>(FourccType("fiel")) && size == currentAtom.dataSize) {
            ctx->offset += static_cast<int64_t>(currentAtom.size);
            return Status::OK;
        }
    }
    // 避免重复extradata
    if (lastTrack_->codecParms.extradataSize > 1 && lastTrack_->codecParms.data) {
        MEDIA_LOG_W("ignoring multiple glbl");
        ctx->offset += static_cast<int64_t>(currentAtom.size);
        return Status::OK;
    }
    // 读取glbl内容到extradata
    auto extradata = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, extradata.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read glbl extradata failed");
    lastTrack_->codecParms.data = std::move(extradata);
    lastTrack_->codecParms.extradataSize = static_cast<int64_t>(currentAtom.dataSize);
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    return Status::OK;
}
// LCOV_EXCL_STOP

Status MPEG4AtomParser::ParseStts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    if (currentAtom.size > MIN_BOX_SIZE) {
        constexpr uint64_t sttsEntryCountOffset = 4;
        constexpr uint64_t sttsEntryCountSize = 4;
        if (currentAtom.dataSize >= sttsEntryCountOffset + sttsEntryCountSize) {
            uint8_t entryCountData[sttsEntryCountSize] = {0};
            Status readRet = dataReader_->ReadUintData(ctx->dataOffset + sttsEntryCountOffset,
                entryCountData, sizeof(entryCountData));
            FALSE_RETURN_V_MSG_E(readRet == Status::OK, readRet, "Read stts entry count failed");
            if (GetU32Value(entryCountData, sizeof(entryCountData)) == 0) {
                MEDIA_LOG_W("Stts entry count is zero, skipping time to sample parsing");
                return Status::OK;
            }
        }
        Status ret = lastTrack_->sampleHelper->SetTimeToSampleParams(ctx->dataOffset, currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set time to sample params failed");
    } else {
        MEDIA_LOG_W("Stts atom size is too small, skipping time to sample parsing");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseStss(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");

    ctx->offset += static_cast<int64_t>(currentAtom.size);
    Status ret = lastTrack_->sampleHelper->SetSyncSampleParams(ctx->dataOffset, currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set sync sample params failed");
    return ret;
}

Status MPEG4AtomParser::ParseCtts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");

    ctx->offset += static_cast<int64_t>(currentAtom.size);
    Status ret = lastTrack_->sampleHelper->SetCompositionTimeToSampleParams(ctx->dataOffset, currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set composition time to sample params failed");
    return ret;
}

Status MPEG4AtomParser::ParseStsc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize > 0, Status::ERROR_INVALID_DATA, "Empty stsc atom");
    if (currentAtom.size > MIN_BOX_SIZE) {
        Status ret = lastTrack_->sampleHelper->SetSampleToChunkParams(ctx->dataOffset, currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set sample to chunk params failed");
    } else {
        MEDIA_LOG_W("Stsc atom size is too small, skipping sample to chunk parsing");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseStsz(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    FALSE_RETURN_V_MSG_E(currentAtom.type != FourccType("stz2") || currentAtom.dataSize > 0,
        Status::ERROR_INVALID_DATA, "Empty stz2 atom");
    constexpr int64_t minStszSize = 20; // stsz atom最小大小
    if (currentAtom.size >= minStszSize) {
        Status ret = lastTrack_->sampleHelper->SetSampleSizeParams(currentAtom.type, ctx->dataOffset,
            currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set sample size params failed");
    } else {
        MEDIA_LOG_W("Stsz atom size is too small, skipping sample size parsing");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseStco(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += currentAtom.size;
    if (currentAtom.size > MIN_BOX_SIZE) {
        Status ret = lastTrack_->sampleHelper->SetChunkOffsetParams(currentAtom.type, ctx->dataOffset,
            currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set chunk offset params failed");
    } else {
        MEDIA_LOG_W("Stco atom size is too small, skipping chunk offset parsing");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseSdtp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += currentAtom.size;
    Status ret = lastTrack_->sampleHelper->SetSampleDependencyTypeParams(ctx->offset, ctx->dataOffset,
        currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SetSampleDependencyTypeParams failed");
    return Status::OK;
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseTrex(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_TWO, Status::ERROR_INVALID_PARAMETER, "Invalid trex depth");
    ctx->offset += currentAtom.size;
    uint32_t trackId = 0;
    if (currentAtom.dataSize >= TREX_TRACK_ID_SIZE) {
        uint8_t trackIdData[TREX_TRACK_ID_SIZE];
        Status ret = dataReader_->ReadUintData(ctx->dataOffset + TREX_TRACK_ID_OFFSET, trackIdData,
            sizeof(trackIdData));
        if (ret == Status::OK) {
            trackId = GetU32Value(trackIdData);
        } else {
            MEDIA_LOG_W("Failed to read trex trackId");
        }
    }
    auto track = firstTrack_;
    while (track != nullptr && static_cast<uint32_t>(track->trackId) != trackId) {
        track = track->next;
    }
    if (track && track->sampleHelper) {
        Status ret = track->sampleHelper->SetTrackExtendsParams(ctx->dataOffset, currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set track extends params failed");
    } else {
        MEDIA_LOG_W("Track id " PUBLIC_LOG_U32 " not found or has no sampleHelper", trackId);
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseSidx(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == 0, Status::ERROR_INVALID_DATA, "Invalid sidx depth");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_SIDX_SIZE && currentAtom.dataSize <= MAX_SIDX_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid sidx atom size");

    auto headerInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    std::shared_ptr<Track> currentTrack = nullptr;
    uint8_t version = 0;
    Status ret = ReadSidxHeaderInfo(currentAtom, ctx, headerInfo, currentTrack, version);
    if (ret != Status::OK) {
        return ret;
    }
    uint64_t entryOffset = 0;
    ret = ParseSidxContext(currentAtom, ctx, headerInfo.get(), version, entryOffset);
    if (ret != Status::OK) {
        return ret;
    }

    if (currentTrack != nullptr) {
        currentTrack->sidxReferencedSize = 0;
    }
    ret = ParseSidxEntries(headerInfo.get(), entryOffset, currentAtom.dataSize, sidxCtx_, currentTrack);
    CheckSidxReferenceEnd();
    if (currentTrack != nullptr) {
        MEDIA_LOG_D("current track index" PUBLIC_LOG_U32 " sidx duration: " PUBLIC_LOG_U64,
            currentTrack->trackIndex, currentTrack->sidxDuration);
    }
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    hasSidxBox_ = true;
    return ret;
}

Status MPEG4AtomParser::ReadSidxHeaderInfo(MPEG4Atom currentAtom, ParseContext* ctx,
    std::unique_ptr<uint8_t[]>& headerInfo, std::shared_ptr<Track>& currentTrack, uint8_t& version)
{
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sidx header info failed");
    version = headerInfo[0];
    FALSE_RETURN_V_MSG_E(version <= 1, Status::ERROR_INVALID_DATA, "Invalid sidx version: " PUBLIC_LOG_U8, version);
    uint32_t referenceId = GetU32Value(&headerInfo[4]);
    currentTrack = FindSidxTrack(referenceId);
    uint32_t timeScale = GetU32Value(&headerInfo[8]);
    FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_INVALID_DATA,
        "Invalid time scale: " PUBLIC_LOG_U32, timeScale);
    return Status::OK;
}

Status MPEG4AtomParser::ParseSidxContext(MPEG4Atom currentAtom, ParseContext* ctx, const uint8_t* headerInfo,
    uint8_t version, uint64_t& entryOffset)
{
    int64_t currentOffset = 12; // Skip version, flags, referenceId, and timeScale
    uint64_t firstOffset = 0;
    constexpr uint64_t firstOffsetV1 = 20;
    constexpr uint64_t firstOffsetV0 = 16;
    if (version == 0) {
        constexpr uint64_t version0HeaderSize = 24;
        FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= version0HeaderSize, Status::ERROR_INVALID_DATA,
            "Invalid sidx v0 atom size");
        sidxCtx_.dts = GetU32Value(&headerInfo[currentOffset]);
        firstOffset = GetU32Value(&headerInfo[firstOffsetV0]);
        currentOffset += sizeof(uint64_t);
    } else if (version == 1) {
        constexpr uint64_t version1HeaderSize = 32;
        FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= version1HeaderSize, Status::ERROR_INVALID_DATA,
            "Invalid sidx v1 atom size");
        sidxCtx_.dts = GetU64Value(&headerInfo[currentOffset]);
        firstOffset = GetU64Value(&headerInfo[firstOffsetV1]);
        currentOffset += sizeof(sidxCtx_.dts) + sizeof(firstOffset);
    }
    int64_t skipSize = 2;
    uint64_t sidxEndOffset = 0;
    uint64_t firstMoofOffset = 0;
    if (ctx->offset < 0 || __builtin_add_overflow(static_cast<uint64_t>(ctx->offset), currentAtom.size,
        &sidxEndOffset) || __builtin_add_overflow(sidxEndOffset, firstOffset, &firstMoofOffset) ||
        firstMoofOffset > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        MEDIA_LOG_E("Sidx first moof offset overflow");
        return Status::ERROR_INVALID_DATA;
    }
    sidxCtx_.firstMoofOffset = static_cast<int64_t>(firstMoofOffset);
    sidxCtx_.entryCount = GetU16Value(&headerInfo[currentOffset + skipSize]);
    FALSE_RETURN_V_MSG_E(sidxCtx_.entryCount > 0 && sidxCtx_.entryCount <= MAX_SIDX_ENTRY_COUNT,
        Status::ERROR_INVALID_DATA, "Invalid sidx entry count");
    currentOffset += sizeof(uint32_t);
    uint64_t entriesSize = 0;
    uint64_t requiredSize = 0;
    entryOffset = static_cast<uint64_t>(currentOffset);
    if (__builtin_mul_overflow(static_cast<uint64_t>(sidxCtx_.entryCount), static_cast<uint64_t>(SIDX_ENTRY_SIZE),
        &entriesSize) || __builtin_add_overflow(entryOffset, entriesSize, &requiredSize) ||
        currentAtom.dataSize < requiredSize) {
        MEDIA_LOG_E("Invalid sidx data size: " PUBLIC_LOG_U64 ", expected at least " PUBLIC_LOG_U64,
            currentAtom.dataSize, requiredSize);
        return Status::ERROR_INVALID_DATA;
    }
    return Status::OK;
}

std::shared_ptr<MPEG4AtomParser::Track> MPEG4AtomParser::FindSidxTrack(uint32_t referenceId)
{
    auto currentTrack = firstTrack_;
    while (currentTrack && static_cast<uint32_t>(currentTrack->trackId) != referenceId) {
        currentTrack = currentTrack->next;
    }
    return currentTrack;
}

void MPEG4AtomParser::CheckSidxReferenceEnd()
{
    uint64_t dataSourceSize = 0;
    if (dataSource_ != nullptr && dataSource_->GetSize(dataSourceSize) == Status::OK &&
        sidxCtx_.firstMoofOffset > static_cast<int64_t>(dataSourceSize)) {
        hasIncompleteSidxReference_ = true;
        MEDIA_LOG_D("Sidx reference end " PUBLIC_LOG_D64 " exceeds file size " PUBLIC_LOG_U64,
            sidxCtx_.firstMoofOffset, dataSourceSize);
    }
}

Status MPEG4AtomParser::ParseSidxEntries(const uint8_t* headerInfo, uint64_t entryOffset, uint64_t bufferSize,
    SidxParseContext& sidxCtx, std::shared_ptr<Track> currentTrack)
{
    uint64_t entriesSize = 0;
    uint64_t entriesEndOffset = 0;
    if (__builtin_mul_overflow(static_cast<uint64_t>(sidxCtx.entryCount), static_cast<uint64_t>(SIDX_ENTRY_SIZE),
        &entriesSize) || __builtin_add_overflow(entryOffset, entriesSize, &entriesEndOffset) ||
        entriesEndOffset > bufferSize) {
        MEDIA_LOG_E("Invalid sidx entries size: entry offset " PUBLIC_LOG_U64 ", count " PUBLIC_LOG_U32
            ", buffer size " PUBLIC_LOG_U64, entryOffset, sidxCtx.entryCount, bufferSize);
        return Status::ERROR_INVALID_DATA;
    }
    for (uint32_t i = 0; i < sidxCtx.entryCount; ++i) {
        uint64_t currentEntryOffset = 0;
        Status ret = GetSidxEntryOffset(entryOffset, bufferSize, i, currentEntryOffset);
        if (ret != Status::OK) {
            return ret;
        }
        ret = ParseSidxEntry(headerInfo, currentEntryOffset, sidxCtx, currentTrack, i);
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::GetSidxEntryOffset(uint64_t entryOffset, uint64_t bufferSize, uint32_t index,
    uint64_t& currentEntryOffset)
{
    uint64_t entryEndOffset = 0;
    uint64_t currentEntriesSize = 0;
    if (__builtin_mul_overflow(static_cast<uint64_t>(index) + 1, static_cast<uint64_t>(SIDX_ENTRY_SIZE),
        &currentEntriesSize) || __builtin_add_overflow(entryOffset, currentEntriesSize, &entryEndOffset) ||
        entryEndOffset > bufferSize) {
        MEDIA_LOG_E("Invalid sidx entry offset: index " PUBLIC_LOG_U32 ", buffer size " PUBLIC_LOG_U64,
            index, bufferSize);
        return Status::ERROR_INVALID_DATA;
    }
    currentEntryOffset = entryEndOffset - SIDX_ENTRY_SIZE;
    return Status::OK;
}

Status MPEG4AtomParser::ParseSidxEntry(const uint8_t* headerInfo, uint64_t currentEntryOffset,
    SidxParseContext& sidxCtx, std::shared_ptr<Track> currentTrack, uint32_t index)
{
    uint32_t size = GetU32Value(&headerInfo[currentEntryOffset]);
    uint32_t duration = GetU32Value(&headerInfo[currentEntryOffset + sizeof(uint32_t)]);
    if (size & 0x80000000) {
        MEDIA_LOG_W("Sub sidx not supported");
    }
    uint32_t referencedSize = size & 0x7FFFFFFF;
    int64_t safeReferencedSize = static_cast<int64_t>(referencedSize);
    int64_t safeDuration = static_cast<int64_t>(duration);
    MEDIA_LOG_D("Entry " PUBLIC_LOG_U32 ": offset=" PUBLIC_LOG_D64 ", size="
        PUBLIC_LOG_U32 ", duration=" PUBLIC_LOG_U32, index, sidxCtx.firstMoofOffset, referencedSize, duration);
    int64_t firstMoofOffset = sidxCtx.firstMoofOffset;
    bool found = std::any_of(fragmentEntry_.begin(), fragmentEntry_.end(),
        [firstMoofOffset](const FragmentEntry& entry) {
            return entry.moofOffset == firstMoofOffset;
        });
    if (!found) {
        FALSE_RETURN_V_MSG_E(sidxCtx.dts <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()),
            Status::ERROR_INVALID_DATA, "Sidx dts overflow");
        fragmentEntry_.emplace_back(FragmentEntry {
            .firstDts = static_cast<int64_t>(sidxCtx.dts),
            .moofOffset = sidxCtx.firstMoofOffset,
            .duration = safeDuration,
            .hasRead = false
        });
    }
    if (currentTrack) {
        if (__builtin_add_overflow(currentTrack->sidxDuration, safeDuration, &currentTrack->sidxDuration) ||
            __builtin_add_overflow(currentTrack->sidxReferencedSize, safeReferencedSize,
                &currentTrack->sidxReferencedSize)) {
            MEDIA_LOG_E("Sidx track info overflow");
            return Status::ERROR_INVALID_DATA;
        }
    }
    if (__builtin_add_overflow(sidxCtx.firstMoofOffset, safeReferencedSize, &sidxCtx.firstMoofOffset) ||
        __builtin_add_overflow(sidxCtx.dts, static_cast<uint64_t>(duration), &sidxCtx.dts)) {
        MEDIA_LOG_E("Sidx context overflow");
        return Status::ERROR_INVALID_DATA;
    }
    return Status::OK;
}

Status MPEG4AtomParser::FindNextMoof(int64_t& headerOffset, uint32_t& atomSize)
{
    Status ret = Status::OK;
    int32_t atomType = 0;
    constexpr uint32_t atomHeaderSize = 8;
    constexpr uint32_t typeOffset = 4;
    do {
        auto atomHeader = std::make_unique<uint8_t[]>(atomHeaderSize);
        FALSE_RETURN_V_MSG_E(atomHeader != nullptr, Status::ERROR_NO_MEMORY, "atomHeader is nullptr");
        ret = dataReader_->ReadUintData(headerOffset, atomHeader.get(), atomHeaderSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Find next moof header info failed");
        atomSize = GetU32Value(atomHeader.get());
        atomType = static_cast<int32_t>(GetU32Value(atomHeader.get() + typeOffset));
        if (atomType != FourccType("moof")) {
            MEDIA_LOG_D("not moof box, go find next");
            FALSE_RETURN_V_MSG_E(atomSize >= atomHeaderSize, Status::ERROR_INVALID_DATA,
                "Invalid atom size " PUBLIC_LOG_U32, atomSize);
            uint32_t atomDataSize = atomSize - atomHeaderSize;
            auto atomData = std::make_unique<uint8_t[]>(atomDataSize);
            FALSE_RETURN_V_MSG_E(atomData != nullptr, Status::ERROR_NO_MEMORY, "atomHeader is nullptr");
            ret = dataReader_->ReadUintData(headerOffset + atomHeaderSize, atomData.get(), atomDataSize);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read atom data failed");
            headerOffset += atomSize;
        }
    } while (atomType != FourccType("moof"));
    return Status::OK;
}

Status MPEG4AtomParser::UpdateFragmentEntryAndOffset(bool inEntry, uint32_t fragIndex, FragmentEntry& frag,
    int64_t moofOffset, int64_t& currentOffset)
{
    if (inEntry) {
        fragmentEntry_[fragIndex].hasRead = true;
    } else {
        while (fragIndex < fragmentEntry_.size() && moofOffset > fragmentEntry_[fragIndex].moofOffset) {
            ++fragIndex;
        }
        frag.hasRead = true;
        fragmentEntry_.insert(fragmentEntry_.begin() + fragIndex, frag);
    }

    Status ret = Status::OK;
    uint32_t typeOffset = 4;
    if (seekable_ == Seekable::UNSEEKABLE) {
        uint8_t atomHeader[8];
        uint32_t size = 0;
        int32_t type = 0;
        do {
            ret = dataReader_->ReadUintData(currentOffset, atomHeader, sizeof(atomHeader));
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read next atom header failed");
            size = GetU32Value(&atomHeader[0]);
            type = static_cast<int32_t>(GetU32Value(&atomHeader[typeOffset]));
            currentOffset += size;
        } while (type != FourccType("mdat"));
        fragmentEntry_[fragIndex].nextAtomOffset = currentOffset;
    }

    if (!isDistributedSidx_ && seekable_ == Seekable::SEEKABLE) {
        uint8_t atomHeader[8];
        ret = dataReader_->ReadUintData(currentOffset, atomHeader, sizeof(atomHeader));
        if (ret == Status::END_OF_STREAM) {
            return Status::OK;
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read next atom header failed");
        uint32_t size = GetU32Value(&atomHeader[0]);
        currentOffset += size;
        ret = dataReader_->ReadUintData(currentOffset, atomHeader, sizeof(atomHeader));
        if (ret == Status::END_OF_STREAM) {
            return Status::OK;
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read next atom header failed");
        // 检查下一个atom是否是sidx
        if (GetU32Value(&atomHeader[0x04]) == static_cast<uint32_t>(FourccType("sidx"))) {
            isDistributedSidx_ = true;
            MEDIA_LOG_D("File has distributed sidx, ignore them all");
        }
    }
    return Status::OK;
}

bool MPEG4AtomParser::UpdataMoofInfo(
    MPEG4Atom& atom, Status& ret, int64_t& currentOffset, FragmentEntry& frag, int64_t sidxDts)
{
    switch (atom.type) {
        case FourccType("mfhd"):
            return ProcessMoofMfhd(atom, ret, currentOffset);
        case FourccType("traf"):
            currentOffset += atom.headerSize;
            return true;
        case FourccType("tfhd"):
            return ProcessMoofTfhd(atom, ret, currentOffset);
        case FourccType("tfdt"):
            return ProcessMoofTfdt(atom, ret, currentOffset);
        case FourccType("trun"):
            return ProcessMoofTrun(atom, ret, currentOffset, frag, sidxDts);
        case FourccType("senc"):
            return ProcessMoofSenc(atom, ret, currentOffset);
        case FourccType("saiz"):
            return ProcessMoofSaiz(atom, ret, currentOffset);
        case FourccType("saio"):
            return ProcessMoofSaio(atom, ret, currentOffset);
        default:
            MEDIA_LOG_W("Unknown atom type: " PUBLIC_LOG_S, FourccToString(atom.type).c_str());
            ret = Status::ERROR_INVALID_DATA;
            return true;
    }
}

Status MPEG4AtomParser::ParseMoof(const std::shared_ptr<Track>& track, int64_t offset)
{
    uint32_t atomSize = 0;
    int64_t headerOffset = offset;
    Status ret = FindNextMoof(headerOffset, atomSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Find next moof atom failed");
    hasMoofBox_ = true;
    moofOffset_ = headerOffset;

    FragmentEntry frag;
    frag.moofOffset = moofOffset_;
    uint32_t fragIndex = 0;
    bool inEntry = false;
    if (!fragmentEntry_.empty()) {
        auto it = std::find_if(fragmentEntry_.begin(), fragmentEntry_.end(),
            [this](const FragmentEntry& entry) { return entry.moofOffset == moofOffset_; });
        if (it != fragmentEntry_.end()) {
            inEntry = true;
            FALSE_RETURN_V_NOLOG(!it->hasRead, Status::OK);
            fragIndex = static_cast<uint32_t>(std::distance(fragmentEntry_.begin(), it));
        }
    }
    int64_t sidxDts = inEntry ? fragmentEntry_[fragIndex].firstDts : -1;
    firstTrack_ = (firstTrack_ == nullptr) ? track : firstTrack_;
    int64_t moofEndOffset = moofOffset_ + atomSize;
    int64_t currentOffset = moofOffset_ + sizeof(uint64_t); // Skip size and type
    currentTrack_ = firstTrack_;
    ClearTrackSaizSaioState();
    while (currentOffset < moofEndOffset) {
        MPEG4Atom atom;
        uint8_t atomHeader[8];
        atom.headerSize = sizeof(atomHeader);
        ret = dataReader_->ReadUintData(currentOffset, atomHeader, atom.headerSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read moof atom header failed");
        atom.size = GetU32Value(&atomHeader[0]);
        atom.type = static_cast<int32_t>(GetU32Value(&atomHeader[ATOM_TYPE_OFFSET]));
        FALSE_RETURN_V_MSG_E(
            currentOffset + static_cast<int64_t>(atom.size) <= moofEndOffset, Status::ERROR_INVALID_DATA,
            "Invalid moof size: " PUBLIC_LOG_U64 ", moof end offset: " PUBLIC_LOG_D64, atom.size, moofEndOffset);
        FALSE_RETURN_V_MSG_E(atom.size >= atom.headerSize, Status::ERROR_INVALID_DATA, "atom.size < atom.headerSize");
        atom.dataSize = atom.size - atom.headerSize;
        atom.dataOffset = currentOffset + static_cast<int64_t>(atom.headerSize); // Skip size and type
        MEDIA_LOG_D("Processing box: " PUBLIC_LOG_S ", size: " PUBLIC_LOG_U64 " at offset " PUBLIC_LOG_D64,
            FourccToString(atom.type).c_str(), atom.dataSize, atom.dataOffset);
        FALSE_RETURN_V_NOLOG(UpdataMoofInfo(atom, ret, currentOffset, frag, sidxDts), ret);
    }
    ret = UpdateFragmentEntryAndOffset(inEntry, fragIndex, frag, moofOffset_, currentOffset);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Update fragment entry and offset failed");
    PadSampleEncryptionEntries();
    return Status::OK;
}

void MPEG4AtomParser::ClearTrackSaizSaioState()
{
    auto track = firstTrack_;
    while (track) {
        track->currentFragmentIndexBase = track->sampleHelper ?
            static_cast<uint32_t>(track->sampleHelper->GetSampleCount()) : 0;
        track->drmInfo.auxiliaryInfoDefaultSize = 0;
        track->drmInfo.auxiliaryInfoSizes.clear();
        track->drmInfo.auxiliaryOffsets.clear();
        track->drmInfo.auxiliaryInfoSampleCount = 0;
        track->drmInfo.auxiliaryOffsetsCount = 0;
        track = track->next;
    }
}

void MPEG4AtomParser::PadSampleEncryptionEntries()
{
    auto track = firstTrack_;
    while (track) {
        if (track->sampleHelper) {
            size_t expectedSize = track->sampleHelper->GetSampleCount();
            if (track->drmInfo.sampleEncryptionEntries.size() < expectedSize) {
                track->drmInfo.sampleEncryptionEntries.resize(expectedSize);
            }
        }
        track = track->next;
    }
}

Status MPEG4AtomParser::ProcessTfhdAtom(const MPEG4Atom& atom)
{
    MEDIA_LOG_D("extract tfhd");
    if (atom.dataOffset < 0 || atom.dataSize < MIN_TFHD_SIZE) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    uint8_t tfhdHeader[8];
    Status ret = dataReader_->ReadUintData(atom.dataOffset, tfhdHeader, sizeof(tfhdHeader));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read header failed");

    uint32_t tfhdFlag = GetU32Value(&tfhdHeader[0]);
    uint32_t trackId = GetU32Value(&tfhdHeader[4]);

    while (currentTrack_ && static_cast<uint32_t>(currentTrack_->trackId) != trackId) {
        currentTrack_ = currentTrack_->next;
    }
    if (!currentTrack_ || !currentTrack_->sampleHelper) {
        MEDIA_LOG_W("tfhd: TrackId " PUBLIC_LOG_U32 " not found or has no sampleHelper", trackId);
        return Status::ERROR_INVALID_DATA;
    }

    int64_t tfhdDataOffset = atom.dataOffset + sizeof(tfhdHeader);
    int64_t dataSize = static_cast<int64_t>(atom.dataSize - sizeof(tfhdHeader));
    ret = currentTrack_->sampleHelper->SetTrackFragmentHeaderParams(tfhdDataOffset, dataSize, tfhdFlag, moofOffset_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set track fragment header params failed");

    return Status::OK;
}
// LCOV_EXCL_STOP

Status MPEG4AtomParser::ParseUdta(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse udta container failed");
    return Status::OK;
}

void MPEG4AtomParser::ParseLocationString(const std::string& locationStr)
{
    std::regex pattern(R"([+-]\d+\.\d+)");
    std::sregex_iterator numbers(locationStr.cbegin(), locationStr.cend(), pattern);
    std::sregex_iterator end;

    const uint32_t minLocationNum = 2; // 至少需要两个数字：纬度和经度
    auto cnt = static_cast<uint32_t>(std::distance(numbers, end));
    FALSE_RETURN_MSG_D(cnt >= minLocationNum, "Info format error");
    SetToFormatIfConvertSuccess(mediaInfo_.general, Tag::MEDIA_LATITUDE, numbers->str(), ConvertValueType::FLOAT);
    SetToFormatIfConvertSuccess(mediaInfo_.general, Tag::MEDIA_LONGITUDE, (++numbers)->str(), ConvertValueType::FLOAT);
    if (cnt > minLocationNum) {
        SetToFormatIfConvertSuccess(
            mediaInfo_.general, Tag::MEDIA_ALTITUDE, (++numbers)->str(), ConvertValueType::FLOAT);
    }
}

std::string GetCoverMimeType(uint32_t dataType)
{
    switch (dataType) {
        case 0x0D:
            return MimeType::IMAGE_JPG;
        case 0x0E:
            return MimeType::IMAGE_PNG;
        case 0x1B:
            return MimeType::IMAGE_BMP;
        default:
            MEDIA_LOG_W("Unknown cover type: " PUBLIC_LOG_U32, dataType);
            return "";
    }
}

bool MPEG4AtomParser::ParseJpegDimensions(const uint8_t* data, uint64_t size, int32_t& width, int32_t& height)
{
    if (data == nullptr || size < JPEG_SIGNATURE_SIZE || data[0] != 0xFF || data[1] != JPEG_SOI) {
        return false;
    }
    uint64_t offset = JPEG_MARKER_PREFIX_SIZE;
    while (offset + JPEG_MIN_SEGMENT_SCAN_SIZE < size) {
        if (data[offset] != 0xFF) {
            ++offset;
            continue;
        }
        uint8_t marker = data[offset + 1];
        offset += JPEG_MARKER_PREFIX_SIZE;
        if (marker == JPEG_SOI || marker == JPEG_EOI) {
            continue;
        }
        FALSE_RETURN_V_NOLOG(offset + JPEG_SEGMENT_LENGTH_SIZE <= size, false);
        uint16_t segmentSize = GetU16Value(data + offset);
        if (segmentSize < JPEG_SEGMENT_LENGTH_SIZE || offset + segmentSize > size) {
            return false;
        }
        bool isSofMarker = (marker >= JPEG_SOF_MIN_PRIMARY && marker <= JPEG_SOF_MAX_PRIMARY) ||
            (marker >= JPEG_SOF_MIN_SECONDARY && marker <= JPEG_SOF_MAX_SECONDARY) ||
            (marker >= JPEG_SOF_MIN_TERTIARY && marker <= JPEG_SOF_MAX_TERTIARY) ||
            (marker >= JPEG_SOF_MIN_QUATERNARY && marker <= JPEG_SOF_MAX_QUATERNARY);
        if (isSofMarker && segmentSize >= JPEG_SOF_MIN_SEGMENT_SIZE) {
            height = GetU16Value(data + offset + JPEG_SOF_HEIGHT_OFFSET);
            width = GetU16Value(data + offset + JPEG_SOF_WIDTH_OFFSET);
            return true;
        }
        offset += segmentSize;
    }
    return false;
}

Status MPEG4AtomParser::ParseCover(MPEG4Atom currentAtom, uint32_t dataType, uint64_t stringSize, ParseContext* ctx)
{
    std::string mimeType = GetCoverMimeType(dataType);
    FALSE_RETURN_V_MSG_W(!mimeType.empty(),
        Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, "Unsupported track: type[%{public}u]", dataType);
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(stringSize, "covr"), Status::ERROR_INVALID_DATA,
        "Invalid cover data size");
    auto coverData = std::make_unique<uint8_t[]>(stringSize);
    FALSE_RETURN_V_MSG_E(coverData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate cover data buffer");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + 16, coverData.get(), stringSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read cover data failed");

    auto coverTrack = std::make_shared<Track>();
    FALSE_RETURN_V_MSG_E(coverTrack != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate cover track");
    coverTrack->cache = std::make_shared<SampleCache>();
    FALSE_RETURN_V_MSG_E(coverTrack->cache != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate cover track cache");

    coverTrack->trackIndex = trackCount_;
    coverTrack->codecParms.trackType = VIDEO_TYPE;
    coverTrack->codecParms.extradataSize = static_cast<int64_t>(stringSize);
    coverTrack->sampleHelper = std::make_shared<MPEG4SampleHelper>();
    FALSE_RETURN_V_MSG_E(coverTrack->sampleHelper != nullptr, Status::ERROR_NO_MEMORY,
        "Failed to allocate cover sample helper");
    coverTrack->sampleHelper->mimeType_ = mimeType;

    Meta coverMeta;
    coverMeta.Set<Tag::MIME_TYPE>(mimeType);
    coverMeta.Set<Tag::MEDIA_TYPE>(MediaType::VIDEO);
    coverMeta.Set<Tag::MEDIA_START_TIME>(static_cast<int64_t>(0));

    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    if (mimeType == MimeType::IMAGE_JPG) {
        ParseJpegDimensions(coverData.get(), stringSize, videoWidth, videoHeight);
    }
    coverMeta.Set<Tag::VIDEO_WIDTH>(videoWidth);
    coverMeta.Set<Tag::VIDEO_HEIGHT>(videoHeight);
    std::vector<uint8_t> cover(stringSize);
    cover.assign(coverData.get(), coverData.get() + stringSize);
    coverMeta.Set<Tag::MEDIA_COVER>(std::move(cover));
    mediaInfo_.tracks.emplace_back(std::move(coverMeta));
    ++trackCount_;

    if (lastTrack_) {
        lastTrack_->next = coverTrack;
    } else {
        firstTrack_ = coverTrack;
    }
    lastTrack_ = coverTrack;
    return ret;
}

Status MPEG4AtomParser::ParseUserDataString(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    std::string key = GetMetadataKey(currentAtom.type, ctx);
    if (key.empty() && currentAtom.type != FourccType("covr")) {
        std::string atomTypeStr = FourccToString(currentAtom.type);
        MEDIA_LOG_W("Unknown metadata type: " PUBLIC_LOG_S, atomTypeStr.c_str());
        return Status::OK;
    }

    std::string value;
    Status ret = Status::OK;
    if (ctx->founditunesMetadata && currentAtom.dataSize > DEFAULT_HEAD_SIZE) {
        ret = ParseItunesMetadata(currentAtom, key, ctx);
    } else {
        ret = ParseTraditionalMetadata(currentAtom, key, ctx);
    }
    ctx->isUserMeta = false;
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse metadata string failed");
    return ret;
}

std::string MPEG4AtomParser::GetMetadataKey(uint32_t atomType, ParseContext* ctx)
{
    // 如果是动态key（iTunes metadata with keys box）
    if (ctx && ctx->foundHdlrMdta && !ctx->metaKeys.empty()) {
        // atomType在iTunes格式中实际是大端序的索引，需要字节序转换
        uint32_t index = atomType;
        if (index > 0 && index < ctx->metaKeysCount) {
            std::string key = ctx->metaKeys[index];
            if (!key.empty()) {
                ctx->isUserMeta = true;
                MEDIA_LOG_D("Found dynamic key[" PUBLIC_LOG_U32 "]: " PUBLIC_LOG_S, index, key.c_str());
                return key;
            }
        } else if (atomType != static_cast<uint32_t>(FourccType("covr"))) {
            std::string atomTypeStr = FourccToString(atomType);
            MEDIA_LOG_W("Dynamic key index out of range: " PUBLIC_LOG_S, atomTypeStr.c_str());
        }
    }

    static std::map<uint32_t, std::string> keyMap = {
        { FourccType("\251nam"), "title" },
        { FourccType("\251ART"), "artist" }, // "\251ART" and "\251aut" refer the same value
        { FourccType("\251aut"), "artist" },
        { FourccType("\251alb"), "album" },
        { FourccType("aART"),    "album_artist" },
        { FourccType("\251day"), "date" },
        { FourccType("\251cmt"), "comment" },
        { FourccType("\251inf"), "comment" },
        { FourccType("\251gen"), "genre" },
        { FourccType("\251cpy"), "copyright" },
        { FourccType("cprt"),    "copyright" },
        { FourccType("\251com"), "composer" },
        { FourccType("\251wrt"), "composer" },
        { FourccType("\251lyr"), "lyrics" },
        { FourccType("\251xyz"), "location" },
        { FourccType("desc"),    "description" },
        { FourccType("\251too"), "encoder" },
        { FourccType("\251enc"), "encoder" },
        { FourccType("\251swr"), "encoder" },
    };

    auto it = keyMap.find(atomType);
    return (it != keyMap.end()) ? it->second : "";
}

Status MPEG4AtomParser::ParseItunesMetadata(MPEG4Atom currentAtom, std::string& key, ParseContext* ctx)
{
    uint8_t dataHeader[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, dataHeader, sizeof(dataHeader));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read iTunes metadata header failed");

    uint32_t dataSize = GetU32Value(&dataHeader[0]);
    uint32_t dataTag = GetU32Value(&dataHeader[4]);
    constexpr uint32_t headerSize = 16; // size + type
    if (dataTag == static_cast<uint32_t>(FourccType("data")) &&
        dataSize <= currentAtom.dataSize && dataSize >= headerSize) {
        uint8_t typeInfo[8];
        ret = dataReader_->ReadUintData(ctx->dataOffset + sizeof(typeInfo), typeInfo, sizeof(typeInfo));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data type info failed");

        uint32_t dataType = GetU32Value(&typeInfo[0]);
        uint64_t stringSize = dataSize - headerSize;
        if (currentAtom.type == FourccType("covr") || (!key.empty() && key == "com.apple.quicktime.artwork")) {
            return ParseCover(currentAtom, dataType, stringSize, ctx);
        }
        ret = ReadMetadataString(key, ctx->dataOffset + headerSize, stringSize, dataType, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read iTunes metadata string failed");
    } else {
        MEDIA_LOG_W("Invalid iTunes metadata format: dataTag=" PUBLIC_LOG_U32, dataTag);
    }

    return Status::OK;
}

Status MPEG4AtomParser::ParseTraditionalMetadata(MPEG4Atom currentAtom, std::string& key, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_TRADITIONAL_METADATA_SIZE, Status::ERROR_INVALID_DATA,
        "Invalid traditional metadata size");
    uint8_t lengthData[2];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, lengthData, sizeof(lengthData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read string length failed");

    uint16_t stringLength = GetU16Value(lengthData);
    constexpr uint16_t defaultSkipSize = 4;
    if (stringLength > currentAtom.dataSize - defaultSkipSize) {
        // 作为原始数据处理
        ret = ReadMetadataString(key, ctx->dataOffset, currentAtom.dataSize, 0, ctx);
    } else {
        // 跳过语言代码
        ret = ReadMetadataString(key, ctx->dataOffset + defaultSkipSize, stringLength, 0, ctx);
    }

    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read traditional metadata string failed");
    return Status::OK;
}

bool ParseSignedInteger(uint8_t* data, uint64_t size, int32_t& out)
{
    FALSE_RETURN_V_MSG_E(data != nullptr, false, "ParseSignedInteger: data is null");
    int32_t value = 0;
    switch (size) {
        case 0x01:
            value = static_cast<int8_t>(data[0]);
            break;
        case 0x02:
            value = static_cast<int16_t>(GetU16Value(data));
            break;
        case 0x03:
            value = static_cast<int32_t>((GetU32Value(data) & 0xFFFFFF00) >> 0x08);
            if (static_cast<uint32_t>(value) & 0x800000) {
                value = static_cast<int32_t>(static_cast<uint32_t>(value) | 0xFF000000);
            }
            break;
        case 0x04:
            value = static_cast<int32_t>(GetU32Value(data));
            break;
        default:
            return false;
    }
    out = value;
    return true;
}

bool ParseUnSignedInteger(uint8_t* data, uint64_t size, uint32_t& out)
{
    FALSE_RETURN_V_MSG_E(data != nullptr, false, "ParseUnSignedInteger: data is null");
    uint32_t value = 0;
    switch (size) {
        case 0x01:
            value = static_cast<uint8_t>(data[0]);
            break;
        case 0x02:
            value = static_cast<uint16_t>(GetU16Value(data));
            break;
        case 0x03:
            value = static_cast<uint32_t>((GetU32Value(data) & 0xFFFFFF00) >> 0x08);
            if (value & 0x800000) {
                value |= 0xFF000000;
            }
            break;
        case 0x04:
            value = static_cast<uint32_t>(GetU32Value(data));
            break;
        default:
            return false;
    }
    out = value;
    return true;
}

// LCOV_EXCL_START
std::string DecodeMacString(const uint8_t* data, size_t size)
{
    FALSE_RETURN_V_MSG_E(data != nullptr && size > 0, "", "DecodeMacString: data is null");

    std::string result;
    result.reserve(size);
    constexpr size_t asciiStart = 32; // ASCII 可打印字符的起始值
    constexpr size_t asciiEnd = 126; // ASCII 可打印字符的结束值

    for (size_t i = 0; i < size; ++i) {
        if (data[i] == 0) {
            break; // 遇到空字符结束
        }
        if (data[i] >= asciiStart && data[i] <= asciiEnd) {
            // ASCII 可打印字符
            result += static_cast<char>(data[i]);
        } else {
            // 非 ASCII 字符，可以选择跳过或转换
            result += '?';
        }
    }
    return result;
}
// LCOV_EXCL_STOP

bool MPEG4AtomParser::SetMetadataValue(const std::string& key, const std::string& value, bool isTrackMetadata)
{
    static const std::map<std::string, TagType> keyToTagMap = {
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
        {"aigc",          Tag::MEDIA_AIGC},
        {"encoder",       Tag::MEDIA_ENCODER},
    };

    auto it = keyToTagMap.find(Converter::ToLower(key));
    if (it != keyToTagMap.end()) {
        std::string resultStr = "";
        if (!Converter::IsUTF8(value) && Converter::IsGBK(value.c_str())) {
            resultStr = Converter::ConvertGBKToUTF8(value);
        }
        resultStr = resultStr.length() > 0 ? resultStr : value;
        if (resultStr.length() > 0) {
            mediaInfo_.general.SetData(it->second, resultStr);
            if (isTrackMetadata && it->second == Tag::MEDIA_TITLE && IsLastTrackValid()) {
                mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_TITLE>(resultStr);
            }
        }
        return true;
    } else if (key == "location") {
        ParseLocationString(value);
        return true;
    }
    return false;
}

Status MPEG4AtomParser::SetMetadata(std::string& key, uint8_t* buffer, uint64_t size, uint32_t dataType)
{
    FALSE_RETURN_V_NOLOG(userFormat_ != nullptr, Status::ERROR_NO_MEMORY);
    switch (dataType) {
        case 0x00: { // reserved buffer
            std::vector<uint8_t> bufferValue(buffer, buffer + size);
            userFormat_->SetData(key, bufferValue);
            break;
        }
        case 0x15:
        case 0x43: { // BE signed integer
            int32_t value = 0;
            if (ParseSignedInteger(buffer, size, value)) {
                userFormat_->SetData(key, value);
            }
            break;
        }
        case 0x16: { // BE unsigned integer
            uint32_t value = 0;
            if (ParseUnSignedInteger(buffer, size, value)) {
                userFormat_->SetData(key, value);
            }
            break;
        }
        case 0x17: { // BE float32
            if (size >= MIN_FLOAT_SIZE) {
                uint32_t uintValue = GetU32Value(buffer);
                float floatValue = *reinterpret_cast<float*>(&uintValue);
                userFormat_->SetData(key, floatValue);
            }
            break;
        }
        case 0x03: { // MAC encoded
            std::string macStringValue = DecodeMacString(buffer, size);
            if (!macStringValue.empty()) {
                userFormat_->SetData(key, macStringValue);
            }
            break;
        }
        default: { // UTF-8 string
            FALSE_RETURN_V_MSG_D(
                dataType <= 1 || dataType == SUPPORT_DATA_TYPE, Status::OK, "Unsupport meta %{public}d", dataType);
            std::string stringValue = std::string(reinterpret_cast<char*>(buffer), size);
            if (!stringValue.empty()) {
                userFormat_->SetData(key, stringValue);
            }
            break;
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::ReadMetadataString(
    std::string& key, int64_t offset, uint64_t size, uint32_t dataType, ParseContext* parseCtx)
{
    FALSE_RETURN_V_NOLOG(size > 0, Status::OK);
    FALSE_RETURN_V_MSG_E(size <= std::min(MPEG4_MAX_BOX_DATA_SIZE, static_cast<uint64_t>(SIZE_MAX - 1)),
        Status::ERROR_INVALID_DATA, "Invalid metadata size");
    auto buffer = std::make_unique<uint8_t[]>(size + 1);
    FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate metadata buffer");
    Status ret = dataReader_->ReadUintData(offset, buffer.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read metadata string data failed");

    buffer[size] = '\0';
    const bool isTrackMeta = parseCtx != nullptr && lastTrack_ != nullptr &&
        lastTrack_->codecParms.trackType == SUBTITLE_TYPE && parseCtx->path.size() >= 4 &&
        parseCtx->path[0] == FourccType("moov") && parseCtx->path[1] == FourccType("trak") &&
        (parseCtx->path[2] == FourccType("udta") || parseCtx->path[2] == FourccType("ilst"));
    const bool isUserMeta = parseCtx != nullptr && parseCtx->isUserMeta;
    if (SetMetadataValue(key, std::string(reinterpret_cast<char*>(buffer.get()), size), isTrackMeta) && !isUserMeta) {
        return Status::OK;
    }
    return SetMetadata(key, buffer.get(), size, dataType);
}

Status MPEG4AtomParser::ParseMeta(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("Container atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    bool isFileLevelGltfMeta = depth == MPEG4_ROOT_DEPTH &&
        ctx->compatibleBrands.count(FourccType("glti")) > 0;
    if (currentAtom.dataSize < sizeof(uint32_t) || (depth == MPEG4_ROOT_DEPTH && !isFileLevelGltfMeta)) {
        // 数据太小，或者在root级别，直接跳过
        ctx->offset = endOffset;
        return Status::OK;
    }
    // 检查是否直接以hdlr开始（检查前8字节是否为合理的hdlr header）
    uint8_t testBytes[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, testBytes, sizeof(testBytes));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read meta test bytes failed");

    uint32_t firstSize = GetU32Value(&testBytes[0]);
    uint32_t firstType = GetU32Value(&testBytes[4]);
    if (firstType == static_cast<uint32_t>(FourccType("hdlr")) &&
        firstSize > sizeof(testBytes) && firstSize <= currentAtom.dataSize) {
        // 直接以hdlr开始，没有version/flags
        ctx->offset = ctx->dataOffset;
        MEDIA_LOG_D("Meta starts with hdlr, no version/flags");
    } else {
        // 有version/flags，跳过4字节
        ctx->offset = ctx->dataOffset + sizeof(uint32_t);
        MEDIA_LOG_D("Meta has version/flags, skipping 4 bytes");
    }
    // 解析子atoms
    while (ctx->offset < endOffset) {
        ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse child at offset " PUBLIC_LOG_D64, ctx->offset);
    }
    if (isFileLevelGltfMeta) {
        TrySetGltfOffset(ctx);
    }
    ctx->offset = endOffset;
    return Status::OK;
}

Status MPEG4AtomParser::ParseKeys(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE, Status::ERROR_INVALID_DATA, "Invalid atom size");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "keys"), Status::ERROR_INVALID_DATA,
        "Invalid keys data size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    // skip version and flags (4 bytes)
    auto keysInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, keysInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read keys atom info failed");
    uint32_t keysCount = GetU32Value(&keysInfo[4]);
    MEDIA_LOG_D("Parsing keys atom with count: " PUBLIC_LOG_U32, keysCount);
    FALSE_RETURN_V_MSG_E(keysCount <= MAX_METADATA_KEYS_COUNT, Status::ERROR_INVALID_DATA,
        "The 'keys' atom with too many keys: " PUBLIC_LOG_U32, keysCount);
    FALSE_RETURN_V_MSG_E(keysCount <= UINT_MAX / sizeof(std::string) - 1, Status::ERROR_INVALID_DATA,
        "The 'keys' atom with the invalid key count: " PUBLIC_LOG_U32, keysCount);
    FALSE_RETURN_V_MSG_E(keysCount <= (currentAtom.dataSize - DEFAULT_HEAD_SIZE) / sizeof(uint64_t),
        Status::ERROR_INVALID_DATA, "Invalid keys count: " PUBLIC_LOG_U32, keysCount);
    // 预分配空间，索引从1开始，0不使用
    ctx->metaKeys.resize(keysCount + 1);
    ctx->metaKeysCount = keysCount + 1;
    // 解析keys
    uint64_t keysOffset = 8;
    for (uint32_t i = 1; i <= keysCount; ++i) {
        FALSE_RETURN_V_MSG_E(keysOffset <= currentAtom.dataSize &&
            currentAtom.dataSize - keysOffset >= sizeof(uint64_t), Status::ERROR_INVALID_DATA,
            "Invalid keys offset");
        uint32_t keySize = GetU32Value(&keysInfo[keysOffset]);
        uint32_t keyType = GetU32Value(&keysInfo[keysOffset + sizeof(uint32_t)]);
        FALSE_RETURN_V_MSG_E(keySize >= sizeof(uint64_t), Status::ERROR_INVALID_DATA,
            "Invalid key size: " PUBLIC_LOG_U32, keySize);
        keysOffset += sizeof(uint64_t);
        uint32_t strLen = keySize - sizeof(uint64_t);
        FALSE_RETURN_V_MSG_E(keysOffset <= currentAtom.dataSize && strLen <= currentAtom.dataSize - keysOffset,
            Status::ERROR_INVALID_DATA, "Invalid key string length: " PUBLIC_LOG_U32, strLen);
        if (keyType != static_cast<uint32_t>(FourccType("mdta"))) {
            keysOffset += strLen;
            continue;
        }
        FALSE_RETURN_V_MSG_W(
            strLen > 0, Status::ERROR_INVALID_DATA, "Invalid key string length: " PUBLIC_LOG_U32, strLen);
        std::string keyStr(reinterpret_cast<char*>(&keysInfo[keysOffset]), strLen);
        // 确保字符串以null结尾
        keyStr = keyStr.substr(0, keyStr.find('\0'));
        ctx->metaKeys[i] = keyStr;
        keysOffset += strLen;
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseIlst(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->founditunesMetadata = true;
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse ilst container failed");

    ctx->founditunesMetadata = false;
    return ret;
}

Status MPEG4AtomParser::ParseLoci(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_LOCI_SIZE, Status::ERROR_INVALID_DATA, "loci atom too small");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "loci"), Status::ERROR_INVALID_DATA,
        "Invalid loci data size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);

    auto lociInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, lociInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read loci atom info failed");

    uint64_t offset = 6; // 跳过version+flags(4) + language(2)
    uint64_t remainingLen = currentAtom.dataSize - offset;

    // 跳过地点名称字符串（以null结尾）
    while (offset < currentAtom.dataSize && lociInfo[offset] != 0) {
        ++offset;
    }

    FALSE_RETURN_V_MSG_E(offset < currentAtom.dataSize, Status::ERROR_INVALID_DATA, "loci place not null-terminated");

    ++offset; // 跳过null terminator
    remainingLen = currentAtom.dataSize - offset;
    // 检查剩余数据是否足够（role(1) + coordinates(12) = 13字节）
    FALSE_RETURN_V_MSG_E(remainingLen >= 13, Status::ERROR_INVALID_DATA, "loci need at least 13");
    ++offset; // 跳过role (1字节)

    int32_t longitudeRaw = static_cast<int32_t>(GetU32Value(&lociInfo[offset]));
    int32_t latitudeRaw = static_cast<int32_t>(GetU32Value(&lociInfo[offset + 4]));
    int32_t altitudeRaw = static_cast<int32_t>(GetU32Value(&lociInfo[offset + 8]));
    float longitude = longitudeRaw / static_cast<float>(CONVERT_SCALE);
    float latitude = latitudeRaw / static_cast<float>(CONVERT_SCALE);
    float altitude = altitudeRaw / static_cast<float>(CONVERT_SCALE);

    std::ostringstream locationStream;
    locationStream << std::fixed <<
                      std::setprecision(PRECISION) <<
                      std::showpos <<
                      std::setfill('0') <<
                      std::setw(LATITUDE_WIDTH) << latitude <<
                      std::setw(LONGITUDE_WIDTH) << longitude;
    if (altitude) {
        locationStream << std::setprecision(ALTITUDE_PRECISION) << altitude;
    }
    std::string locationStr = locationStream.str();
    ParseLocationString(locationStr);
    return ret;
}

static const char* GetID3v1GenreString(uint8_t genreNumber)
{
    static constexpr std::array<const char*, 192> id3v1_genre_table = {{
        "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal",
        "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial",
        "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
        "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House", "Game",
        "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass", "Soul", "Punk", "Space", "Meditative",
        "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
        "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
        "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret", "New Wave",
        "Psychedelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka",
        "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk", "Folk-Rock", "National Folk", "Swing",
        "Fast Fusion", "Bebop", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
        "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band", "Chorus",
        "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music", "Sonata",
        "Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
        "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo",
        "A Cappella", "Euro-House", "Dance Hall", "Goa", "Drum & Bass", "Club-House", "Hardcore Techno",
        "Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk", "Beat", "Christian Gangsta Rap", "Heavy Metal",
        "Black Metal", "Crossover", "Contemporary Christian", "Christian Rock", "Merengue", "Salsa", "Thrash Metal",
        "Anime", "Jpop", "Synthpop", "Abstract", "Art Rock", // "Art Rock" is a genre conforming to ID3v1.
        "Baroque", "Bhangra", "Big Beat", "Breakbeat", "Chillout", "Downtempo", "Dub", "EBM", "Eclectic", "Electro",
        "Electroclash", "Emo", "Experimental", "Garage", "Global", "IDM", "Illbient", "Industro-Goth", "Jam Band",
        "Krautrock", "Leftfield", "Lounge", "Math Rock", "New Romantic", "Nu-Breakz", "Post-Punk", "Post-Rock",
        "Psytrance", "Shoegaze", "Space Rock", "Trop Rock", "World Music", "Neoclassical", "Audiobook",
        "Audio Theatre", "Neue Deutsche Welle", "Podcast", "Indie Rock", "G-Funk", "Dubstep", "Garage Rock", "Psybient"
    }};
    return (genreNumber < id3v1_genre_table.size()) ? id3v1_genre_table[genreNumber] : nullptr;
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseUdtaGenre(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    // gnre atom应该至少有2字节数据
    if (currentAtom.dataSize >= 2) {
        uint8_t genreData[2];
        Status ret = dataReader_->ReadUintData(ctx->dataOffset, genreData, 2);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read gnre data failed");

        // 跳过第一个字节
        uint8_t genreNumber = genreData[1];
        if (genreNumber >= 1 && genreNumber <= ID3V1_GENRE_MAX + 1) {
            const char* genreString = GetID3v1GenreString(genreNumber - 1);
            if (genreString != nullptr) {
                mediaInfo_.general.Set<Tag::MEDIA_GENRE>(std::string(genreString));
            } else {
                MEDIA_LOG_W("Failed to get genre string for number: " PUBLIC_LOG_U8, genreNumber);
            }
        } else {
            MEDIA_LOG_W("Genre number out of range: " PUBLIC_LOG_U8 " (valid: 1-192)", genreNumber);
        }
    } else {
        MEDIA_LOG_W("gnre atom too small: " PUBLIC_LOG_U64 " bytes", currentAtom.dataSize);
    }
    return Status::OK;
}
// LCOV_EXCL_STOP

Status MPEG4AtomParser::ParseMoovGenre(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_NOLOG(currentAtom.size != 0, Status::OK);
    FALSE_RETURN_V_MSG_E(currentAtom.size <= std::min(MPEG4_MAX_BOX_DATA_SIZE, static_cast<uint64_t>(SIZE_MAX - 1)),
        Status::ERROR_INVALID_DATA, "Invalid genre atom size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    auto genre = std::make_unique<uint8_t[]>(currentAtom.size + 1);
    FALSE_RETURN_V_MSG_E(genre != nullptr, Status::ERROR_NO_MEMORY, "Read moov genre failed");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, genre.get(), currentAtom.size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");

    uint64_t start = 0;
    for (start = 0; start < currentAtom.size; ++start) {
        if (genre[start] != 0) {
            break;
        }
    }
    SetMetadataValue("genre", std::string(reinterpret_cast<char*>(genre.get() + start)));
    return Status::OK;
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseFrma(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= sizeof(uint32_t), Status::ERROR_INVALID_DATA,
        "Invalid frma atom size");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    uint8_t info[sizeof(uint32_t)] = {0};
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, info, sizeof(info));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read frma atom info failed");
    uint32_t originalFormat = GetU32Value(info);
    std::string originalFormatStr = FourccToString(originalFormat);
    MEDIA_LOG_D("Parsing frma atom with original format: " PUBLIC_LOG_S, originalFormatStr.c_str());
    // For encrypted tracks (encv/enca), set mime type based on original format
    if (lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr &&
        (lastTrack_->sampleHelper->mimeType_.empty() ||
         lastTrack_->sampleHelper->mimeType_ == MimeType::INVALID_TYPE)) {
        std::string mimeType = GetMimeType(originalFormat);
        if (mimeType != MimeType::INVALID_TYPE) {
            SetMime(mimeType);
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseSchi(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("Atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    ctx->offset = ctx->dataOffset;
    while (ctx->offset < endOffset) {
        Status ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse schi container failed");
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA, "Invalid schi size");
    return Status::OK;
}

Status MPEG4AtomParser::ParseSchm(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_SCHM_SIZE, Status::ERROR_INVALID_DATA, "schm box too small");
    uint8_t schmData[12];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, schmData, sizeof(schmData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read schm data failed");
    uint32_t schemeType = GetU32Value(&schmData[4]);
    lastTrack_->drmInfo.schemeType = schemeType;
    MEDIA_LOG_I("ParseSchm: scheme_type=%.4s (0x%08X)", FourccToString(schemeType).c_str(), schemeType);
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseSinf(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("Atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    ctx->offset = ctx->dataOffset;
    while (ctx->offset < endOffset) {
        Status ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse sinf container failed");
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA, "Invalid sinf size");
    return Status::OK;
}

Status MPEG4AtomParser::ParsePssh(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_PSSH_SIZE, Status::ERROR_INVALID_DATA, "pssh box too small");
    uint8_t versionFlags[PSSH_VERSION_FLAGS_SIZE];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, versionFlags, sizeof(versionFlags));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pssh version failed");
    uint8_t version = versionFlags[0];
    int64_t uuidOffset = ctx->dataOffset + sizeof(versionFlags);
    uint8_t uuidData[PSSH_UUID_SIZE];
    ret = dataReader_->ReadUintData(uuidOffset, uuidData, sizeof(uuidData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pssh uuid failed");
    int64_t dataOffset = uuidOffset + sizeof(uuidData);
    uint32_t psshDataSize = 0;
    if (version == 0) {
        FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_PSSH_SIZE + sizeof(uint32_t),
            Status::ERROR_INVALID_DATA, "pssh v0 box too small");
        uint8_t sizeData[sizeof(uint32_t)];
        ret = dataReader_->ReadUintData(dataOffset, sizeData, sizeof(sizeData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pssh data size failed");
        psshDataSize = GetU32Value(sizeData);
        dataOffset += sizeof(sizeData);
    } else if (version == 1) {
        FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_PSSH_SIZE + sizeof(uint32_t),
            Status::ERROR_INVALID_DATA, "pssh v1 box too small");
        uint8_t kidCountData[sizeof(uint32_t)];
        ret = dataReader_->ReadUintData(dataOffset, kidCountData, sizeof(kidCountData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pssh kid count failed");
        uint32_t kidCount = GetU32Value(kidCountData);
        dataOffset += sizeof(kidCountData);
        FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_PSSH_SIZE + sizeof(uint32_t) +
            static_cast<uint64_t>(kidCount) * PSSH_UUID_SIZE + sizeof(uint32_t),
            Status::ERROR_INVALID_DATA, "pssh v1 box too small");
        dataOffset += static_cast<int64_t>(kidCount) * PSSH_UUID_SIZE;
        uint8_t sizeData[sizeof(uint32_t)];
        ret = dataReader_->ReadUintData(dataOffset, sizeData, sizeof(sizeData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pssh data size failed");
        psshDataSize = GetU32Value(sizeData);
        dataOffset += sizeof(sizeData);
    }
    ret = BuildPsshInfo(ctx->offset, uuidData, static_cast<uint32_t>(currentAtom.size));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build pssh info failed");
    ctx->offset += currentAtom.size;
    (void) psshDataSize;
    return Status::OK;
}

Status MPEG4AtomParser::BuildPsshInfo(int64_t boxOffset, const uint8_t uuidData[PSSH_UUID_SIZE],
    uint32_t boxSize)
{
    Mpeg4PsshInfo psshInfo;
    psshInfo.uuidLen = PSSH_UUID_SIZE;
    errno_t memcpyRet = memcpy_s(psshInfo.uuid, sizeof(psshInfo.uuid), uuidData, PSSH_UUID_SIZE);
    FALSE_RETURN_V_MSG_E(memcpyRet == EOK, Status::ERROR_INVALID_OPERATION, "Copy pssh uuid failed");
    psshInfo.psshData.resize(boxSize);
    Status ret = dataReader_->ReadUintData(boxOffset, psshInfo.psshData.data(), boxSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pssh box data failed");
    psshInfo.psshLen = boxSize;
    psshList_.push_back(std::move(psshInfo));
    return Status::OK;
}

Status MPEG4AtomParser::ParseTenc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_TENC_SIZE, Status::ERROR_INVALID_DATA, "tenc box too small");
    uint8_t tencData[32];
    uint32_t readSize = std::min(static_cast<uint32_t>(currentAtom.dataSize), static_cast<uint32_t>(sizeof(tencData)));
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, tencData, readSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tenc data failed");
    uint8_t version = tencData[0];
    // tenc v0: [version+flags(4)][reserved(1)][reserved(1)][isProtected(1)][perSampleIvSize(1)][KID(16)]
    // tenc v1: [version+flags(4)][patternByte(1)][isProtected(1)][perSampleIvSize(1)][KID(16)]
    uint32_t isProtectedOffset = (version == 0) ? TENC_V0_IS_PROTECTED_OFFSET : TENC_V1_IS_PROTECTED_OFFSET;
    uint32_t perSampleIvSizeOffset = (version == 0) ?
        TENC_V0_PER_SAMPLE_IV_SIZE_OFFSET : TENC_V1_PER_SAMPLE_IV_SIZE_OFFSET;
    uint32_t kidOffset = (version == 0) ? TENC_V0_KID_OFFSET : TENC_V1_KID_OFFSET;
    uint8_t patternByte = tencData[TENC_PATTERN_BYTE_OFFSET]; // valid only for version > 0
    uint8_t defaultIsProtected = tencData[isProtectedOffset];
    uint8_t defaultPerSampleIvSize = tencData[perSampleIvSizeOffset];
    lastTrack_->drmInfo.isEncrypted = (defaultIsProtected != 0);
    lastTrack_->drmInfo.perSampleIvSize = defaultPerSampleIvSize;
    errno_t memcpyRet = memcpy_s(lastTrack_->drmInfo.defaultCencInfo.keyId,
        sizeof(lastTrack_->drmInfo.defaultCencInfo.keyId), &tencData[kidOffset], PSSH_UUID_SIZE);
    FALSE_RETURN_V_MSG_E(memcpyRet == EOK, Status::ERROR_INVALID_OPERATION, "Copy tenc keyId failed");
    lastTrack_->drmInfo.defaultCencInfo.keyIdLen = PSSH_UUID_SIZE;
    bool ivSizeInvalid = false;
    if (defaultPerSampleIvSize != 0 && defaultPerSampleIvSize != DRM_IV_SIZE_8 &&
            defaultPerSampleIvSize != DRM_IV_SIZE_16) {
        MEDIA_LOG_W("Unsupported per_sample_iv_size: " PUBLIC_LOG_D32 ", skip constant IV read",
            defaultPerSampleIvSize);
        ivSizeInvalid = true;
        lastTrack_->drmInfo.perSampleIvSize = 0;
    }
    if (version > 0) {
        lastTrack_->drmInfo.defaultCencInfo.encryptBlocks = (patternByte >> TENC_PATTERN_BYTE_SHIFT)
            & TENC_PATTERN_BYTE_MASK;
        lastTrack_->drmInfo.defaultCencInfo.skipBlocks = patternByte & TENC_PATTERN_BYTE_MASK;
    }
    // OHOS_DRM: read default_constant_IV when perSampleIvSize == 0 and isProtected
    // Skip constant IV read when per_sample_iv_size was invalid to avoid reading garbage data
    ret = ReadTencDefaultConstantIV(ctx, kidOffset, defaultIsProtected, defaultPerSampleIvSize, ivSizeInvalid);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tenc default constant IV failed");
    lastTrack_->drmInfo.defaultCencInfo.mode = 0; // AV_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ReadTencDefaultConstantIV(ParseContext* ctx, uint32_t kidOffset,
    uint8_t defaultIsProtected, uint8_t defaultPerSampleIvSize, bool ivSizeInvalid)
{
    // OHOS_DRM: read default_constant_IV when perSampleIvSize == 0 and isProtected
    // tenc v0: KID ends at dataOffset + 24; tenc v1: KID ends at dataOffset + 23
    // Skip constant IV read when per_sample_iv_size was invalid to avoid reading garbage data
    if (defaultIsProtected && defaultPerSampleIvSize == 0 && !ivSizeInvalid) {
        uint32_t ivSizeOffset = kidOffset + PSSH_UUID_SIZE;
        uint8_t ivSizeData[1];
        Status ret = dataReader_->ReadUintData(ctx->dataOffset + ivSizeOffset, ivSizeData, sizeof(ivSizeData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tenc default IV size failed");
        uint8_t defaultIvSize = ivSizeData[0];
        if (defaultIvSize != DRM_IV_SIZE_8 && defaultIvSize != DRM_IV_SIZE_16) {
            MEDIA_LOG_W("Invalid default_constant_IV_size: " PUBLIC_LOG_D32 ", skip constant IV", defaultIvSize);
            defaultIvSize = 0;
        }
        if (defaultIvSize > 0) {
            ret = dataReader_->ReadUintData(ctx->dataOffset + ivSizeOffset + 1,
                lastTrack_->drmInfo.defaultCencInfo.iv, defaultIvSize);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read tenc default IV failed");
            lastTrack_->drmInfo.defaultCencInfo.ivLen = defaultIvSize;
        }
    }
    return Status::OK;
}
// LCOV_EXCL_STOP

const std::vector<MPEG4AtomParser::Mpeg4PsshInfo>& MPEG4AtomParser::GetPsshList() const
{
    return psshList_;
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseSenc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    // fMP4 per-sample encryption info lives in moof, not moov. Skip moov-level senc to avoid
    // polluting sampleEncryptionEntries with stale/default entries that shift moof indices.
    if (depth > 0) {
        ctx->offset += currentAtom.size;
        return Status::OK;
    }
    // If current fragment already has encryption info (from saiz/saio), ignore duplicate senc.
    // Use currentFragmentIndexBase as the boundary: entries before it belong to previous fragments.
    if (lastTrack_->drmInfo.sampleEncryptionEntries.size() > lastTrack_->currentFragmentIndexBase) {
        ctx->offset += currentAtom.size;
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_SENC_SIZE, Status::ERROR_INVALID_DATA, "senc box too small");
    uint8_t headerData[MIN_SENC_SIZE];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerData, sizeof(headerData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read senc header failed");
    uint8_t flags = headerData[3];
    bool useSubsamples = (flags & SENC_FLAGS_SUBSAMPLE_ENCRYPTION) != 0 ||
        (flags & SENC_FLAGS_PIFF_SAMPLE_ENCRYPTION) != 0;
    uint32_t sampleCount = GetU32Value(&headerData[4]);
    uint32_t dataOffset = ctx->dataOffset + sizeof(headerData);
    size_t baseIndex = lastTrack_->drmInfo.sampleEncryptionEntries.size();
    lastTrack_->drmInfo.sampleEncryptionEntries.resize(baseIndex + sampleCount);
    for (uint32_t i = 0; i < sampleCount; ++i) {
        ret = ParseSencSampleEntry(dataOffset, baseIndex + i, useSubsamples,
            ctx->dataOffset + currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse senc sample entry failed");
    }
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseSencSampleEntry(uint32_t& dataOffset, size_t entryIndex, bool useSubsamples,
    uint32_t atomEndOffset)
{
    uint32_t perSampleIvSize = lastTrack_->drmInfo.perSampleIvSize;
    if (perSampleIvSize > 0) {
        FALSE_RETURN_V_MSG_E(dataOffset + perSampleIvSize <= atomEndOffset,
            Status::ERROR_INVALID_DATA, "senc iv read overflow");
        lastTrack_->drmInfo.sampleEncryptionEntries[entryIndex].iv.resize(perSampleIvSize);
        Status ret = dataReader_->ReadUintData(dataOffset,
            lastTrack_->drmInfo.sampleEncryptionEntries[entryIndex].iv.data(), perSampleIvSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read senc iv failed");
        dataOffset += perSampleIvSize;
    }
    if (!useSubsamples) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(dataOffset + SUBSAMPLE_COUNT_SIZE <= atomEndOffset,
        Status::ERROR_INVALID_DATA, "senc subsample count read overflow");
    uint8_t subsampleCountData[SUBSAMPLE_COUNT_SIZE];
    Status ret = dataReader_->ReadUintData(dataOffset, subsampleCountData, sizeof(subsampleCountData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read senc subsample count failed");
    uint16_t subsampleCount = GetU16Value(subsampleCountData);
    dataOffset += sizeof(subsampleCountData);
    lastTrack_->drmInfo.sampleEncryptionEntries[entryIndex].subSamples.resize(subsampleCount);
    for (uint16_t j = 0; j < subsampleCount; ++j) {
        FALSE_RETURN_V_MSG_E(dataOffset + SUBSAMPLE_ENTRY_SIZE <= atomEndOffset,
            Status::ERROR_INVALID_DATA, "senc subsample read overflow");
        uint8_t subsampleData[SUBSAMPLE_ENTRY_SIZE];
        ret = dataReader_->ReadUintData(dataOffset, subsampleData, sizeof(subsampleData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read senc subsample failed");
        lastTrack_->drmInfo.sampleEncryptionEntries[entryIndex].subSamples[j].clearHeaderLen =
            GetU16Value(&subsampleData[0]);
        lastTrack_->drmInfo.sampleEncryptionEntries[entryIndex].subSamples[j].payLoadLen =
            GetU32Value(&subsampleData[SUBSAMPLE_CLEAR_HEADER_LEN_SIZE]);
        dataOffset += sizeof(subsampleData);
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseSaiz(const MPEG4Atom& atom)
{
    FALSE_RETURN_V_MSG_E(currentTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    if (currentTrack_->drmInfo.auxiliaryInfoSampleCount > 0) {
        MEDIA_LOG_E("Duplicate saiz atom within same moof");
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_E(atom.dataSize >= MIN_SAIZ_SIZE, Status::ERROR_INVALID_DATA, "saiz box too small");

    uint8_t headerData[MIN_SAIZ_SIZE];
    Status ret = dataReader_->ReadUintData(atom.dataOffset, headerData, sizeof(headerData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saiz header failed");

    uint8_t version = headerData[0];
    (void) version;
    uint32_t flags = (static_cast<uint32_t>(headerData[1]) << 16) |
        (static_cast<uint32_t>(headerData[2]) << 8) | static_cast<uint32_t>(headerData[3]);

    int64_t dataOffset = atom.dataOffset + PSSH_VERSION_FLAGS_SIZE;

    if (flags & SAIZ_SAIO_FLAGS_AUX_INFO_TYPE) {
        FALSE_RETURN_V_MSG_E(atom.dataSize >= MIN_SAIZ_SIZE + AUX_INFO_TYPE_SIZE,
            Status::ERROR_INVALID_DATA, "saiz box too small for flags=1");
        uint8_t auxInfoData[AUX_INFO_TYPE_SIZE];
        ret = dataReader_->ReadUintData(dataOffset, auxInfoData, sizeof(auxInfoData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saiz aux info type failed");
        uint32_t auxInfoType = GetU32Value(&auxInfoData[0]);
        uint32_t auxInfoParam = GetU32Value(&auxInfoData[sizeof(uint32_t)]);
        dataOffset += AUX_INFO_TYPE_SIZE;

        if (currentTrack_->drmInfo.schemeType != 0) {
            if (auxInfoType != currentTrack_->drmInfo.schemeType) {
                return Status::OK;
            }
            if (auxInfoParam != 0) {
                return Status::OK;
            }
        } else {
            if ((auxInfoType == FourccType("cenc") || auxInfoType == FourccType("cens") ||
                 auxInfoType == FourccType("cbc1") || auxInfoType == FourccType("cbcs")) &&
                auxInfoParam == 0) {
                MEDIA_LOG_E("Saw encrypted saiz without schm/tenc");
                return Status::ERROR_INVALID_DATA;
            } else {
                return Status::OK;
            }
        }
    } else if (currentTrack_->drmInfo.schemeType == 0) {
        return Status::OK;
    }

    ret = ParseSaizBody(dataOffset, atom);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse saiz body failed");
    return Status::OK;
}

Status MPEG4AtomParser::ParseSaizBody(int64_t dataOffset, const MPEG4Atom& atom)
{
    uint64_t minSizeAfterFlags = static_cast<uint64_t>(dataOffset - atom.dataOffset) + SAIZ_SIZE_COUNT_SIZE;
    FALSE_RETURN_V_MSG_E(atom.dataSize >= minSizeAfterFlags,
        Status::ERROR_INVALID_DATA, "saiz box too small for default size and count");

    uint8_t sizeCountData[SAIZ_SIZE_COUNT_SIZE];
    Status ret = dataReader_->ReadUintData(dataOffset, sizeCountData, sizeof(sizeCountData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saiz size and count failed");

    uint8_t defaultSize = sizeCountData[0];
    uint32_t sampleCount = GetU32Value(&sizeCountData[1]);
    dataOffset += SAIZ_SIZE_COUNT_SIZE;

    currentTrack_->drmInfo.auxiliaryInfoDefaultSize = defaultSize;
    currentTrack_->drmInfo.auxiliaryInfoSampleCount = sampleCount;
    currentTrack_->drmInfo.auxiliaryInfoSizes.clear();

    if (defaultSize == 0) {
        uint64_t sizesDataSize = static_cast<uint64_t>(dataOffset - atom.dataOffset) + sampleCount;
        FALSE_RETURN_V_MSG_E(atom.dataSize >= sizesDataSize,
            Status::ERROR_INVALID_DATA, "saiz box too small for individual sizes");
        if (sampleCount > 0) {
            currentTrack_->drmInfo.auxiliaryInfoSizes.resize(sampleCount);
            ret = dataReader_->ReadUintData(dataOffset, currentTrack_->drmInfo.auxiliaryInfoSizes.data(), sampleCount);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saiz individual sizes failed");
        }
    }

    if (currentTrack_->drmInfo.auxiliaryOffsetsCount > 0) {
        return ParseAuxiliaryInfo(currentTrack_);
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseSaio(const MPEG4Atom& atom)
{
    FALSE_RETURN_V_MSG_E(currentTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    if (currentTrack_->drmInfo.auxiliaryOffsetsCount > 0) {
        MEDIA_LOG_E("Duplicate saio atom within same moof");
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_E(atom.dataSize >= MIN_SAIO_SIZE, Status::ERROR_INVALID_DATA, "saio box too small");

    uint8_t headerData[MIN_SAIO_SIZE];
    Status ret = dataReader_->ReadUintData(atom.dataOffset, headerData, sizeof(headerData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saio header failed");

    uint8_t version = headerData[0];
    uint32_t flags = (static_cast<uint32_t>(headerData[1]) << 16) |
        (static_cast<uint32_t>(headerData[2]) << 8) | static_cast<uint32_t>(headerData[3]);

    int64_t dataOffset = atom.dataOffset + PSSH_VERSION_FLAGS_SIZE;

    if (flags & SAIZ_SAIO_FLAGS_AUX_INFO_TYPE) {
        FALSE_RETURN_V_MSG_E(atom.dataSize >= MIN_SAIO_SIZE + AUX_INFO_TYPE_SIZE,
            Status::ERROR_INVALID_DATA, "saio box too small for flags=1");
        uint8_t auxInfoData[AUX_INFO_TYPE_SIZE];
        ret = dataReader_->ReadUintData(dataOffset, auxInfoData, sizeof(auxInfoData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saio aux info type failed");
        uint32_t auxInfoType = GetU32Value(&auxInfoData[0]);
        uint32_t auxInfoParam = GetU32Value(&auxInfoData[sizeof(uint32_t)]);
        dataOffset += AUX_INFO_TYPE_SIZE;

        if (currentTrack_->drmInfo.schemeType != 0) {
            if (auxInfoType != currentTrack_->drmInfo.schemeType) {
                MEDIA_LOG_I("Ignoring saio box with non-matching aux_info_type");
                return Status::OK;
            }
            if (auxInfoParam != 0) {
                MEDIA_LOG_I("Ignoring saio box with non-zero aux_info_type_parameter");
                return Status::OK;
            }
        } else {
            if ((auxInfoType == FourccType("cenc") || auxInfoType == FourccType("cens") ||
                 auxInfoType == FourccType("cbc1") || auxInfoType == FourccType("cbcs")) &&
                auxInfoParam == 0) {
                MEDIA_LOG_E("Saw encrypted saio without schm/tenc");
                return Status::ERROR_INVALID_DATA;
            } else {
                return Status::OK;
            }
        }
    } else if (currentTrack_->drmInfo.schemeType == 0) {
        return Status::OK;
    }

    ret = ReadSaioOffsetEntries(dataOffset, version, atom);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saio offset entries failed");

    return Status::OK;
}

Status MPEG4AtomParser::ReadSaioOffsetEntries(int64_t dataOffset, uint8_t version, const MPEG4Atom& atom)
{
    FALSE_RETURN_V_MSG_E(atom.dataSize >= static_cast<uint64_t>(dataOffset - atom.dataOffset) + SAIO_ENTRY_COUNT_SIZE,
        Status::ERROR_INVALID_DATA, "saio box too small for entry count");

    uint8_t countData[SAIO_ENTRY_COUNT_SIZE];
    Status ret = dataReader_->ReadUintData(dataOffset, countData, sizeof(countData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saio entry count failed");

    uint32_t entryCount = GetU32Value(countData);
    dataOffset += SAIO_ENTRY_COUNT_SIZE;

    uint32_t offsetSize = (version == 0) ? 4 : 8;
    FALSE_RETURN_V_MSG_E(atom.dataSize >= static_cast<uint64_t>(dataOffset - atom.dataOffset) +
        static_cast<uint64_t>(entryCount) * offsetSize,
        Status::ERROR_INVALID_DATA, "saio box too small for offsets");

    currentTrack_->drmInfo.auxiliaryOffsets.clear();
    currentTrack_->drmInfo.auxiliaryOffsets.resize(entryCount);
    currentTrack_->drmInfo.auxiliaryOffsetsCount = entryCount;

    int64_t baseOffset = moofOffset_;
    for (uint32_t i = 0; i < entryCount; ++i) {
        uint8_t offsetData[8];
        ret = dataReader_->ReadUintData(dataOffset, offsetData, offsetSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read saio offset failed");
        if (version == 0) {
            currentTrack_->drmInfo.auxiliaryOffsets[i] = GetU32Value(offsetData) + baseOffset;
        } else {
            currentTrack_->drmInfo.auxiliaryOffsets[i] = GetU64Value(offsetData) + baseOffset;
        }
        dataOffset += offsetSize;
    }

    if (currentTrack_->drmInfo.auxiliaryInfoSampleCount > 0) {
        return ParseAuxiliaryInfo(currentTrack_);
    }

    return Status::OK;
}

Status MPEG4AtomParser::ParseAuxiliaryInfo(const std::shared_ptr<Track>& track)
{
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is null");
    auto& drmInfo = track->drmInfo;

    // If current fragment already has encryption info (from senc), ignore duplicate saiz/saio.
    if (drmInfo.sampleEncryptionEntries.size() > track->currentFragmentIndexBase) {
        return Status::OK;
    }

    if (drmInfo.auxiliaryOffsetsCount != 1) {
        MEDIA_LOG_E("Multiple auxiliary info chunks are not supported");
        return Status::ERROR_INVALID_DATA;
    }

    uint32_t sampleCount = drmInfo.auxiliaryInfoSampleCount;
    if (sampleCount == 0) {
        return Status::OK;
    }

    int64_t auxiliaryOffset = static_cast<int64_t>(drmInfo.auxiliaryOffsets[0]);

    size_t baseIndex = drmInfo.sampleEncryptionEntries.size();
    drmInfo.sampleEncryptionEntries.resize(baseIndex + sampleCount);

    for (uint32_t i = 0; i < sampleCount; ++i) {
        uint32_t sampleInfoSize = drmInfo.auxiliaryInfoDefaultSize
            ? drmInfo.auxiliaryInfoDefaultSize
            : (i < drmInfo.auxiliaryInfoSizes.size() ? drmInfo.auxiliaryInfoSizes[i] : 0);
        Status ret = ParseAuxiliarySampleEntry(auxiliaryOffset, baseIndex + i, sampleInfoSize, track);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse auxiliary sample entry failed");
    }

    return Status::OK;
}

Status MPEG4AtomParser::ParseAuxiliarySampleEntry(int64_t& auxiliaryOffset, size_t entryIndex,
    uint32_t sampleInfoSize, const std::shared_ptr<Track>& track)
{
    auto& drmInfo = track->drmInfo;
    if (drmInfo.perSampleIvSize == 0 && sampleInfoSize <= drmInfo.perSampleIvSize) {
        return Status::OK;
    }
    auto& entry = drmInfo.sampleEncryptionEntries[entryIndex];
    if (drmInfo.perSampleIvSize > 0) {
        FALSE_RETURN_V_MSG_E(sampleInfoSize >= drmInfo.perSampleIvSize,
            Status::ERROR_INVALID_DATA, "Aux info size smaller than IV size");
        entry.iv.resize(drmInfo.perSampleIvSize);
        Status ret = dataReader_->ReadUintData(auxiliaryOffset, entry.iv.data(), drmInfo.perSampleIvSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read auxiliary IV failed");
        auxiliaryOffset += drmInfo.perSampleIvSize;
    }

    if (sampleInfoSize <= drmInfo.perSampleIvSize) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(sampleInfoSize >= drmInfo.perSampleIvSize + SUBSAMPLE_COUNT_SIZE,
        Status::ERROR_INVALID_DATA, "Aux info size too small for subsample count");
    uint8_t subsampleCountData[SUBSAMPLE_COUNT_SIZE];
    Status ret = dataReader_->ReadUintData(auxiliaryOffset, subsampleCountData, sizeof(subsampleCountData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read auxiliary subsample count failed");
    uint16_t subsampleCount = GetU16Value(subsampleCountData);
    auxiliaryOffset += SUBSAMPLE_COUNT_SIZE;

    uint32_t expectedSize = drmInfo.perSampleIvSize + SUBSAMPLE_COUNT_SIZE +
        subsampleCount * SUBSAMPLE_ENTRY_SIZE;
    FALSE_RETURN_V_MSG_E(sampleInfoSize >= expectedSize,
        Status::ERROR_INVALID_DATA, "Aux info size mismatch for subsamples");

    entry.subSamples.resize(subsampleCount);
    for (uint16_t j = 0; j < subsampleCount; ++j) {
        uint8_t subsampleData[SUBSAMPLE_ENTRY_SIZE];
        ret = dataReader_->ReadUintData(auxiliaryOffset, subsampleData, sizeof(subsampleData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read auxiliary subsample failed");
        entry.subSamples[j].clearHeaderLen = GetU16Value(&subsampleData[0]);
        entry.subSamples[j].payLoadLen = GetU32Value(&subsampleData[SUBSAMPLE_CLEAR_HEADER_LEN_SIZE]);
        auxiliaryOffset += SUBSAMPLE_ENTRY_SIZE;
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseChan(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_BOX_SIZE, Status::ERROR_INVALID_DATA, "Chan corrupted");
    FALSE_RETURN_V_MSG_E(CheckBoxDataAllocSize(currentAtom.dataSize, "chan"), Status::ERROR_INVALID_DATA,
        "Invalid chan data size");
    const uint32_t chanEntrySize = 20;
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    auto chanData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, chanData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read chan data failed");

    const uint32_t entryOffset = 12;
    uint32_t layoutTag = GetU32Value(&chanData[4]);
    uint32_t bitmap = GetU32Value(&chanData[8]);
    uint32_t entryNum = GetU32Value(&chanData[12]);
    uint32_t currentOffset = entryOffset;
    int64_t channelSets = 0;
    int64_t channelLayoutMask = 0;
    uint64_t requiredSize = entryOffset + static_cast<uint64_t>(entryNum) * chanEntrySize;
    FALSE_RETURN_V(currentAtom.dataSize >= requiredSize, Status::OK);

    for (uint32_t i = 0; i < entryNum; ++i) {
        uint32_t channelLabel = GetU32Value(&chanData[currentOffset]);
        currentOffset += chanEntrySize;
        if (layoutTag == 0) {
            int64_t channelSet = ReadChannelSet(channelLabel);
            if (channelSet == 0) {
                channelSets = 0;
                break;
            }
            uint64_t tempChannelSets = static_cast<uint64_t>(channelSets);
            channelSets = static_cast<int64_t>(tempChannelSets | tempChannelSets);
        }
    }
    channelLayoutMask = layoutTag == 0 ? (channelSets ? channelSets : 0) : GetChannelLayoutByBitmap(layoutTag, bitmap);
    if (channelLayoutMask) {
        auto audioParser = std::make_shared<MPEG4AudioParser>();
        FALSE_RETURN_V_MSG_E(audioParser != nullptr, Status::ERROR_NO_MEMORY, "Failed create audioParser");
        AudioChannelLayout layout = audioParser->FindValidChannelLayout(static_cast<uint64_t>(channelLayoutMask));
        if (layout == AudioChannelLayout::UNKNOWN) {
            int32_t channelCount = 0;
            FALSE_RETURN_V_MSG_E(
                mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount),
                Status::ERROR_INVALID_DATA, "Channel count missed");
            layout = audioParser->GetDefaultChannelLayout(channelCount);
        }
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
    }
    return Status::OK;
}
// LCOV_EXCL_STOP

Status MPEG4AtomParser::ParseIdat(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    if (UnderFileLevelMetaPath(ctx->path, depth)) {
        ctx->fileMetaIdatPayloadOffset = ctx->dataOffset;
        TrySetGltfOffset(ctx);
    }
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    return Status::OK;
}

Status MPEG4AtomParser::MPEG4ParseHeader(const std::shared_ptr<DataSource>& source, Seekable seekable,
    int64_t readTimeoutMs, Plugins::DemuxerMode mode)
{
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");
    MediaAVCodec::AVCodecTrace trace("mpeg4_init");
    dataSource_ = source;
    seekable_ = seekable;
    demuxerMode_ = mode;
    pendingSampleTableStatus_ = Status::OK;
    dataReader_ = std::make_shared<DemuxerDataReader>();
    FALSE_RETURN_V_MSG_E(dataReader_ != nullptr, Status::ERROR_NO_MEMORY, "Failed create dataReader");
    dataReader_->SetDataReader(source);
    if (readTimeoutMs >= 0) {
        dataReader_->SetReadTimeoutForInitSeek(readTimeoutMs);
    }
    Status ret = Status::OK;
    uint64_t dataSourceSize = 0;
    Status sizeRet = dataSource_->GetSize(dataSourceSize);
    while ((seekable == Seekable::UNSEEKABLE && !(moovFound_ && hasMdatBox_)) ||
           (seekable == Seekable::SEEKABLE && !hasMoofBox_) ||
           (seekable == Seekable::SEEKABLE && hasMoofBox_)) {
        if (sizeRet == Status::OK && ctx_.offset >= static_cast<int64_t>(dataSourceSize)) {
            MEDIA_LOG_D("Reached end of data source at offset: " PUBLIC_LOG_D64, ctx_.offset);
            break;
        }
        int64_t originalOffset = ctx_.offset;
        int32_t depth = 0;
        ret = MPEG4ParseAtom(depth, &ctx_);
        if (ret != Status::OK) {
            MEDIA_LOG_W("Parse error occurred: " PUBLIC_LOG_D32, static_cast<int32_t>(ret));
            FALSE_RETURN_V_NOLOG(ret != Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, ret);
            break;
        } else if (ctx_.offset <= originalOffset) {
            MEDIA_LOG_E("Parse header failed");
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    FALSE_RETURN_V_MSG_E(moovFound_, Status::ERROR_INVALID_DATA, "MPEG4 parse failed");
    FALSE_RETURN_V_MSG_E(pendingSampleTableStatus_ == Status::OK, pendingSampleTableStatus_,
        "MPEG4 parse failed due to invalid sample tables");
    CalculateMPEG4Attributes();
    return ret;
}

void MPEG4AtomParser::NotifyInitializationCompleted()
{
    if (dataReader_ != nullptr) {
        dataReader_->NotifyInitializationCompleted();
    }
}

// LCOV_EXCL_START
Status MPEG4AtomParser::ParseClli(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_CLLI_SIZE, Status::OK, "Clli atom too small");
    uint8_t info[4];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, info, sizeof(info));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read chan data failed");
    float maxCLL = static_cast<float>(GetU16Value(&info[0]));
    float maxFALL = static_cast<float>(GetU16Value(&info[2]));
    Cta861 cta861;
    cta861.maxContentLightLevel = maxCLL;
    cta861.maxFrameAverageLightLevel = maxFALL;
    std::vector<uint8_t> cta861Vec(sizeof(cta861));
    cta861Vec.assign(reinterpret_cast<uint8_t*>(&cta861), reinterpret_cast<uint8_t*>(&cta861) + sizeof(cta861));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_STATIC_METADATA_CTA861>(cta861Vec);
    return Status::OK;
}

Status MPEG4AtomParser::ParseMdcv(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(IsLastTrackValid(), Status::ERROR_INVALID_DATA, "Invalid current track");
    ctx->offset += static_cast<int64_t>(currentAtom.size);
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_MDCV_SIZE, Status::OK, "Mdcv atom too small");
    uint8_t info[24];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, info, sizeof(info));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read chan data failed");
    float displayPrimaryGreenX = static_cast<float>(GetU16Value(&info[0]));
    float displayPrimaryGreenY = static_cast<float>(GetU16Value(&info[2]));
    float displayPrimaryBlueX = static_cast<float>(GetU16Value(&info[4]));
    float displayPrimaryBlueY = static_cast<float>(GetU16Value(&info[6]));
    float displayPrimaryRedX = static_cast<float>(GetU16Value(&info[8]));
    float displayPrimaryRedY = static_cast<float>(GetU16Value(&info[10]));
    float whitePointX = static_cast<float>(GetU16Value(&info[12]));
    float whitePointY = static_cast<float>(GetU16Value(&info[14]));
    float maxLuminance = static_cast<float>(GetU32Value(&info[16]));
    float minLuminance = static_cast<float>(GetU32Value(&info[20]));
    Smpte2086 smpte2086;
    smpte2086.displayPrimaryRedX = displayPrimaryRedX * PRIMARY_SCALE;
    smpte2086.displayPrimaryRedY = displayPrimaryRedY * PRIMARY_SCALE;
    smpte2086.displayPrimaryGreenX = displayPrimaryGreenX * PRIMARY_SCALE;
    smpte2086.displayPrimaryGreenY = displayPrimaryGreenY * PRIMARY_SCALE;
    smpte2086.displayPrimaryBlueX = displayPrimaryBlueX * PRIMARY_SCALE;
    smpte2086.displayPrimaryBlueY = displayPrimaryBlueY * PRIMARY_SCALE;
    smpte2086.whitePointX = whitePointX * PRIMARY_SCALE;
    smpte2086.whitePointY = whitePointY * PRIMARY_SCALE;
    smpte2086.maxLuminance = maxLuminance * LUMI_SCALE;
    smpte2086.minLuminance = minLuminance * LUMI_SCALE;
    std::vector<uint8_t> smpte2086Vec(sizeof(smpte2086));
    smpte2086Vec.assign(
        reinterpret_cast<uint8_t*>(&smpte2086), reinterpret_cast<uint8_t*>(&smpte2086) + sizeof(smpte2086));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_STATIC_METADATA_SMPT2086>(smpte2086Vec);
    return Status::OK;
}
// LCOV_EXCL_STOP

void MPEG4AtomParser::InitParseTable()
{
    // 填充解析函数表
    MPEG4ParseTable_ = {
        {FourccType("ftyp"), &MPEG4AtomParser::ParseFtyp}, {FourccType("moov"), &MPEG4AtomParser::ParseMoov},
        {FourccType("wide"), &MPEG4AtomParser::ParseWide}, {FourccType("mdat"), &MPEG4AtomParser::ParseMdat},
        {FourccType("trak"), &MPEG4AtomParser::ParseTrak}, {FourccType("mvhd"), &MPEG4AtomParser::ParseMvhd},
        {FourccType("tkhd"), &MPEG4AtomParser::ParseTkhd}, {FourccType("edts"), &MPEG4AtomParser::ParseEdts},
        {FourccType("elst"), &MPEG4AtomParser::ParseElst}, {FourccType("tref"), &MPEG4AtomParser::ParseTref},
        {FourccType("mdia"), &MPEG4AtomParser::ParseMdia},
        {FourccType("mdhd"), &MPEG4AtomParser::ParseMdhd}, {FourccType("hdlr"), &MPEG4AtomParser::ParseHdlr},
        {FourccType("minf"), &MPEG4AtomParser::ParseMinf}, {FourccType("stbl"), &MPEG4AtomParser::ParseStbl},
        {FourccType("stsd"), &MPEG4AtomParser::ParseStsd}, {FourccType("btrt"), &MPEG4AtomParser::ParseBtrt},
        {FourccType("pasp"), &MPEG4AtomParser::ParsePasp}, {FourccType("esds"), &MPEG4AtomParser::ParseEsds},
        {FourccType("avcC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("hvcC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("vvcC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("vttC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("colr"), &MPEG4AtomParser::ParseColr}, {FourccType("aclr"), &MPEG4AtomParser::ParseAclr},
        {FourccType("glbl"), &MPEG4AtomParser::ParseGlbl}, {FourccType("stts"), &MPEG4AtomParser::ParseStts},
        {FourccType("stss"), &MPEG4AtomParser::ParseStss}, {FourccType("ctts"), &MPEG4AtomParser::ParseCtts},
        {FourccType("stsc"), &MPEG4AtomParser::ParseStsc}, {FourccType("stsz"), &MPEG4AtomParser::ParseStsz},
        {FourccType("stz2"), &MPEG4AtomParser::ParseStsz}, {FourccType("stco"), &MPEG4AtomParser::ParseStco},
        {FourccType("sdtp"), &MPEG4AtomParser::ParseSdtp},
        {FourccType("co64"), &MPEG4AtomParser::ParseStco},
        {FourccType("udta"), &MPEG4AtomParser::ParseUdta},
        {FourccType("meta"), &MPEG4AtomParser::ParseMeta}, {FourccType("ilst"), &MPEG4AtomParser::ParseIlst},
        {FourccType("loci"), &MPEG4AtomParser::ParseLoci},
        {FourccType("wave"), &MPEG4AtomParser::ParseWave}, {FourccType("frma"), &MPEG4AtomParser::ParseFrma},
        {FourccType("schi"), &MPEG4AtomParser::ParseSchi}, {FourccType("schm"), &MPEG4AtomParser::ParseSchm},
        {FourccType("sinf"), &MPEG4AtomParser::ParseSinf},
        {FourccType("pssh"), &MPEG4AtomParser::ParsePssh}, {FourccType("tenc"), &MPEG4AtomParser::ParseTenc},
        {FourccType("senc"), &MPEG4AtomParser::ParseSenc},
        {FourccType("chan"), &MPEG4AtomParser::ParseChan},
        {FourccType("gnre"), &MPEG4AtomParser::ParseMoovGenre},
        {FourccType("idat"), &MPEG4AtomParser::ParseIdat},
        {FourccType("clli"), &MPEG4AtomParser::ParseClli}, {FourccType("mdcv"), &MPEG4AtomParser::ParseMdcv},
    };
}

Status MPEG4AtomParser::ParseAtomHeader(MPEG4Atom& atom, std::string& typeStr, int32_t depth,
    bool &isZeroSizeAtom, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(ctx != nullptr && ctx->offset >= 0 &&
        static_cast<uint64_t>(ctx->offset) <= static_cast<uint64_t>(INT64_MAX) - DEFAULT_HEAD_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid atom header offset");
    uint64_t dataSourceSize = 0;
    if (dataSource_ != nullptr && dataSource_->GetSize(dataSourceSize) == Status::OK) {
        uint64_t offset = static_cast<uint64_t>(ctx->offset);
        FALSE_RETURN_V_MSG_E(offset <= dataSourceSize && dataSourceSize - offset >= DEFAULT_HEAD_SIZE,
            Status::ERROR_INVALID_DATA, "Atom header exceeds data source");
    }
    uint8_t atomInfo[8];
    const uint32_t typeOffset = 4; // type在第5-8字节
    Status ret = dataReader_->ReadUintData(ctx->offset, atomInfo, sizeof(atomInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read atom header info failed");

    atom.size = GetU32Value(&atomInfo[0]);
    atom.type = static_cast<int32_t>(GetU32Value(&atomInfo[typeOffset]));
    typeStr = FourccToString(atom.type);
    ctx->dataOffset = ctx->offset + sizeof(atomInfo);

    ret = ParseAtomSize(atom, depth, isZeroSizeAtom, ctx);
    if (ret != Status::OK || isZeroSizeAtom) {
        return ret;
    }
    FALSE_RETURN_V_MSG_E(atom.size >= sizeof(atomInfo), Status::ERROR_INVALID_DATA, "Invalid atom size");
    FALSE_RETURN_V_MSG_E(ctx->dataOffset >= ctx->offset, Status::ERROR_INVALID_DATA,
        "Invalid atom header offset");
    atom.effectiveSize = atom.size;
    uint64_t headerSize = static_cast<uint64_t>(ctx->dataOffset - ctx->offset);
    return ValidateAtomSourceRange(atom, typeStr, depth, ctx, headerSize);
}

Status MPEG4AtomParser::ParseAtomSize(MPEG4Atom& atom, int32_t depth, bool& isZeroSizeAtom, ParseContext* ctx)
{
    const uint64_t minExtendSize = 16;
    if (atom.size == 1) {
        FALSE_RETURN_V_MSG_E(ctx->dataOffset >= 0 &&
            static_cast<uint64_t>(ctx->dataOffset) <= static_cast<uint64_t>(INT64_MAX) - sizeof(uint64_t),
            Status::ERROR_INVALID_DATA, "Invalid extended atom header offset");
        uint64_t dataSourceSize = 0;
        if (dataSource_ != nullptr && dataSource_->GetSize(dataSourceSize) == Status::OK) {
            uint64_t offset = static_cast<uint64_t>(ctx->dataOffset);
            FALSE_RETURN_V_MSG_E(offset <= dataSourceSize && dataSourceSize - offset >= sizeof(uint64_t),
                Status::ERROR_INVALID_DATA, "Extended atom header exceeds data source");
        }
        uint8_t sizeData[8];
        Status ret = dataReader_->ReadUintData(ctx->dataOffset, sizeData, sizeof(sizeData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read atom extended size failed");
        atom.size = GetU64Value(sizeData, sizeof(sizeData));
        ctx->dataOffset += sizeof(sizeData);
        FALSE_RETURN_V_MSG_E(atom.size >= minExtendSize, Status::ERROR_INVALID_DATA,
            "Invalid extended atom size: " PUBLIC_LOG_U64, atom.size);
    } else if (atom.size == 0) {
        if (depth == 0) {
            uint64_t dataSourceSize = 0;
            Status ret = dataSource_->GetSize(dataSourceSize);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_INVALID_PARAMETER,
                "Failed to get data source size");
            FALSE_RETURN_V_MSG_E(ctx->offset >= 0 && static_cast<uint64_t>(ctx->offset) <= dataSourceSize,
                Status::ERROR_INVALID_DATA, "Invalid zero-size atom offset");
            atom.size = dataSourceSize - static_cast<uint64_t>(ctx->offset);
        } else {
            FALSE_RETURN_V_MSG_E(ctx->offset >= 0 &&
                static_cast<uint64_t>(ctx->offset) <= static_cast<uint64_t>(INT64_MAX) - sizeof(uint32_t),
                Status::ERROR_INVALID_DATA, "Invalid zero-size atom offset");
            ctx->offset += sizeof(uint32_t);
            isZeroSizeAtom = true;
        }
    }
    return Status::OK;
}

Status MPEG4AtomParser::ValidateAtomSourceRange(MPEG4Atom& atom, const std::string& typeStr, int32_t depth,
    ParseContext* ctx, uint64_t atomInfoSize)
{
    FALSE_RETURN_V_MSG_E(ctx != nullptr && ctx->offset >= 0, Status::ERROR_INVALID_DATA, "Invalid atom offset");
    FALSE_RETURN_V_MSG_E(atomInfoSize >= DEFAULT_HEAD_SIZE && atomInfoSize <= atom.size,
        Status::ERROR_INVALID_DATA, "Invalid atom header size");
    uint64_t atomEndOffset = 0;
    if (__builtin_add_overflow(static_cast<uint64_t>(ctx->offset), atom.size, &atomEndOffset)) {
        MEDIA_LOG_E("Atom size overflow at offset: " PUBLIC_LOG_D64, ctx->offset);
        return Status::ERROR_INVALID_DATA;
    }
    uint64_t dataSourceSize = 0;
    if (dataSource_ != nullptr && dataSource_->GetSize(dataSourceSize) == Status::OK) {
        FALSE_RETURN_V_MSG_E(static_cast<uint64_t>(ctx->offset) <= dataSourceSize,
            Status::ERROR_INVALID_DATA, "Invalid atom offset");
        bool atomExceedsSource = atomEndOffset > dataSourceSize;
        uint64_t remainingSize = dataSourceSize - static_cast<uint64_t>(ctx->offset);
        if (atomExceedsSource && depth == MPEG4_ROOT_DEPTH && atom.type == MDAT_ATOM_TYPE) {
            FALSE_RETURN_V_MSG_E(remainingSize >= atomInfoSize, Status::ERROR_INVALID_DATA,
                "Invalid truncated mdat atom header");
            atom.effectiveSize = remainingSize;
            MEDIA_LOG_W("Trim truncated root mdat: offset " PUBLIC_LOG_D64 ", declared size " PUBLIC_LOG_U64
                ", effective size " PUBLIC_LOG_U64 ", source size " PUBLIC_LOG_U64,
                ctx->offset, atom.size, atom.effectiveSize, dataSourceSize);
            return Status::OK;
        }
        if (atomExceedsSource && depth == MPEG4_ROOT_DEPTH && moovFound_ && hasMdatBox_ &&
            MPEG4ParseTable_.find(atom.type) == MPEG4ParseTable_.end()) {
            FALSE_RETURN_V_MSG_E(remainingSize >= atomInfoSize, Status::ERROR_INVALID_DATA,
                "Invalid trailing unknown atom size");
            atom.effectiveSize = remainingSize;
            MEDIA_LOG_W("Trim trailing unknown atom: offset " PUBLIC_LOG_D64 ", type " PUBLIC_LOG_S
                ", source size " PUBLIC_LOG_U64, ctx->offset, typeStr.c_str(), dataSourceSize);
        } else if (atomExceedsSource) {
            MEDIA_LOG_E("Atom exceeds data source: offset " PUBLIC_LOG_D64 ", size " PUBLIC_LOG_U64
                ", source size " PUBLIC_LOG_U64, ctx->offset, atom.size, dataSourceSize);
            return Status::ERROR_INVALID_DATA;
        }
    }
    return Status::OK;
}

void MPEG4AtomParser::SetHdrTypeInfo(MPEG4Atom currentAtom)
{
    FALSE_RETURN_NOLOG(lastTrack_ != nullptr);
    if (currentAtom.type == FourccType("aesd") && lastTrack_->codecParms.trackType == VIDEO_TYPE &&
        IsValidTrackIndex(mediaInfo_, lastTrack_->trackIndex)) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_WIDTH>(0);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_HEIGHT>(0);
    }
    std::set<int32_t> hdrVividBox = {
        FourccType("cuvv")
    };
    std::set<int32_t> dbBox = {
        FourccType("dvcC"), FourccType("dvvC"), FourccType("dvwC"), FourccType("dvh1")
    };
    if (hdrVividBox.count(currentAtom.type)) {
        lastTrack_->codecParms.hdrBoxInfo.haveHdrVividBox = true;
    } else if (dbBox.count(currentAtom.type)) {
        lastTrack_->codecParms.hdrBoxInfo.haveHdrDoblyVisionBox = true;
    }
}

static bool IsToleratedEmptyAtom(int32_t atomType)
{
    switch (atomType) {
        case FourccType("mvhd"):
        case FourccType("tkhd"):
        case FourccType("elst"):
        case FourccType("tref"):
        case FourccType("mdhd"):
        case FourccType("hdlr"):
        case FourccType("btrt"):
        case FourccType("pasp"):
        case FourccType("esds"):
        case FourccType("aclr"):
        case FourccType("ctts"):
        case FourccType("loci"):
        case FourccType("frma"):
        case FourccType("senc"):
        case FourccType("chan"):
        case FourccType("gnre"):
            return true;
        default:
            return false;
    }
}

Status MPEG4AtomParser::MPEG4ParseAtom(int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(dataReader_ != nullptr && ctx != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Invalid params");
    FALSE_RETURN_V_MSG_E(ctx->offset >= 0 && depth <= MAX_DEPTH, Status::ERROR_INVALID_PARAMETER,
        "Invalid offset or depth");

    MPEG4Atom atom;
    std::string atomTypeStr;
    bool isZeroSizeAtom = false;
    Status ret = ParseAtomHeader(atom, atomTypeStr, depth, isZeroSizeAtom, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read and parse atom header failed");
    FALSE_RETURN_V_MSG_E(!isZeroSizeAtom, Status::OK, "Zero size non-root atom, skip it");

    PathAdder pathAdder(&ctx->path, atom.type);
    FALSE_RETURN_V_MSG_E(atom.effectiveSize <= atom.size, Status::ERROR_INVALID_DATA,
        "Invalid atom effective size");
    FALSE_RETURN_V_MSG_E(atom.effectiveSize >= static_cast<uint64_t>(ctx->dataOffset - ctx->offset),
        Status::ERROR_INVALID_DATA, "Invalid atom effective size");
    atom.dataSize = atom.effectiveSize - static_cast<uint64_t>(ctx->dataOffset - ctx->offset);
    if (atom.type != MDAT_ATOM_TYPE && atom.dataSize > MAX_NON_MDAT_ATOM_PAYLOAD_SIZE) {
        MEDIA_LOG_E("Non-mdat atom payload too large: type " PUBLIC_LOG_S ", payload " PUBLIC_LOG_U64
            ", max " PUBLIC_LOG_U64, atomTypeStr.c_str(), atom.dataSize, MAX_NON_MDAT_ATOM_PAYLOAD_SIZE);
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_W(!IsUnsupportedFmp4Atom(atom.type),
        Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, "Unsupported file: fmp4");
    // 使用函数指针查找和调用解析函数
    ParseFunction parseFunc = FindAtomParser(atom, ctx);
    if (parseFunc) {
        if (atom.dataSize == 0 && IsToleratedEmptyAtom(atom.type)) {
            MEDIA_LOG_W("Ignore empty atom: " PUBLIC_LOG_S, atomTypeStr.c_str());
            ctx->offset += static_cast<int64_t>(atom.effectiveSize);
            return Status::OK;
        }
        ret = (this->*parseFunc)(atom, depth, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse atom failed for type: " PUBLIC_LOG_S, atomTypeStr.c_str());
    } else {
        // 无法识别的atom类型，跳过
        SetHdrTypeInfo(atom);
        FALSE_RETURN_V_MSG_E(atom.effectiveSize <= static_cast<uint64_t>(INT64_MAX) &&
            static_cast<uint64_t>(ctx->offset) <= static_cast<uint64_t>(INT64_MAX) - atom.effectiveSize,
            Status::ERROR_INVALID_DATA, "Invalid atom effective size");
        ctx->offset += static_cast<int64_t>(atom.effectiveSize);
    }
    return ret;
}

auto MPEG4AtomParser::FindAtomParser(const MPEG4Atom& atom, ParseContext* ctx) -> ParseFunction
{
    // 首先查找静态解析函数表
    auto it = MPEG4ParseTable_.find(atom.type);
    if (it != MPEG4ParseTable_.end()) {
        return it->second;
    }
    // 查找动态解析函数
    if (ctx->path.size() >= DIRECT_META_PATH_SIZE) {
        // 获取父级atom类型 - path中倒数第二个元素
        uint32_t parentType = ctx->path[ctx->path.size() - DIRECT_META_PATH_SIZE];
        if (parentType == static_cast<uint32_t>(FourccType("udta")) ||
            parentType == static_cast<uint32_t>(FourccType("ilst"))) {
            if (atom.type == FourccType("gnre")) {
                return &MPEG4AtomParser::ParseUdtaGenre;
            } else {
                return &MPEG4AtomParser::ParseUserDataString;
            }
        } else if (parentType == static_cast<uint32_t>(FourccType("meta")) && atom.type == FourccType("keys") &&
                ctx->foundHdlrMdta && ctx->metaKeysCount == 0) {
            return &MPEG4AtomParser::ParseKeys;
        }
    }
    return nullptr;
}


void MPEG4AtomParser::CalculateMPEG4Attributes()
{
    MEDIA_LOG_D("In");
    mediaInfo_.general.Set<Tag::MEDIA_HAS_VIDEO>(false);
    mediaInfo_.general.Set<Tag::MEDIA_HAS_AUDIO>(false);
    mediaInfo_.general.Set<Tag::MEDIA_HAS_SUBTITLE>(false);
    mediaInfo_.general.Set<Tag::MEDIA_HAS_TIMEDMETA>(false);
    mediaInfo_.general.Set<Tag::MEDIA_HAS_AUXILIARY>(false);
    for (auto track = firstTrack_; track; track = track->next) {
        if (track->sampleHelper == nullptr || !IsValidTrackIndex(mediaInfo_, track->trackIndex)) {
            MEDIA_LOG_W("Skip invalid track metadata, trackIndex: " PUBLIC_LOG_U32, track->trackIndex);
            continue;
        }
        if (NeedParseVideoInfo(track->sampleHelper->mimeType_, track->codecParms.trackType)) {
            CalculateVideoFrameRate(track);
            CalculateVideoDelay(track);
        }
        if (NeedParseAudioInfo(track->sampleHelper->mimeType_, track->codecParms.trackType)) {
            int32_t channelCount = 0;
            AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout) &&
                mediaInfo_.tracks[track->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount)) {
                layout = Converter::GetDefaultChannelLayout(channelCount);
                mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
                mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
            }
            mediaInfo_.tracks[track->trackIndex].Set<Tag::AUDIO_BLOCK_ALIGN>(0);
        }
        NormalizeAudioAttributes(track);
        if (track->codecParms.trackType == VIDEO_TYPE && track->sampleHelper != nullptr &&
            IsImageMimeType(track->sampleHelper->mimeType_)) {
            ParseImageTrackData(track);
        } else {
            CalculateTrackBitrates(track);
        }
        if (IsVideoMimeType(track->sampleHelper->mimeType_) &&
            track->sampleHelper != nullptr && track->sampleHelper->GetRawSampleCount() <= 0) {
            mediaInfo_.tracks[track->trackIndex].Remove(Tag::MEDIA_BITRATE);
        }
        SetTrackType(track);
    }

    SetStartTime();
    SetFileDuration();
    SetHEVCDefaultValue();
}

void MPEG4AtomParser::SetTrackType(std::shared_ptr<Track> track)
{
    TagType tagType;
    switch (track->codecParms.trackType) {
        case VIDEO_TYPE: {
            if (track->sampleHelper != nullptr && !IsImageMimeType(track->sampleHelper->mimeType_)) {
                tagType = Tag::MEDIA_HAS_VIDEO;
            }
            break;
        }
        case AUDIO_TYPE:
            tagType = Tag::MEDIA_HAS_AUDIO;
            break;
        case SUBTITLE_TYPE:
            tagType = Tag::MEDIA_HAS_SUBTITLE;
            break;
        case TIMEDMETA_TYPE:
            tagType = Tag::MEDIA_HAS_TIMEDMETA;
            break;
        case AUXILIARY_TYPE:
            tagType = Tag::MEDIA_HAS_AUXILIARY;
            break;
        default:
            MEDIA_LOG_W("Unknown track type for track " PUBLIC_LOG_U32, track->trackIndex);
            return; // 不处理未知类型
    }
    if (!tagType.empty()) {
        mediaInfo_.general.SetData(tagType, true);
    }
    if (track != nullptr && track->sampleHelper != nullptr && track->trackIndex < mediaInfo_.tracks.size()) {
        std::string codecName = GetCodecName(track->sampleHelper->mimeType_);
        if (!codecName.empty()) {
            mediaInfo_.tracks[track->trackIndex].Set<Tag::ORIGINAL_CODEC_NAME>(codecName);
        }
}
}

bool MPEG4AtomParser::ProcessMoofMfhd(MPEG4Atom& atom, Status& ret, int64_t& currentOffset)
{
    constexpr uint64_t mfhdReadSize = 8;
    FALSE_RETURN_V_MSG_E(atom.dataSize >= mfhdReadSize, false, "Invalid mfhd atom size");
    uint8_t mfhdData[mfhdReadSize];
    ret = dataReader_->ReadUintData(atom.dataOffset, mfhdData, sizeof(mfhdData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Read data failed");
    currentOffset += static_cast<int64_t>(atom.size);
    return true;
}

bool MPEG4AtomParser::ProcessMoofTfhd(MPEG4Atom& atom, Status& ret, int64_t& currentOffset)
{
    currentOffset += static_cast<int64_t>(atom.size);
    ret = ProcessTfhdAtom(atom);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Process tfhd atom failed");
    return true;
}

bool MPEG4AtomParser::ProcessMoofTfdt(MPEG4Atom& atom, Status& ret, int64_t& currentOffset)
{
    currentOffset += static_cast<int64_t>(atom.size);
    FALSE_RETURN_V_MSG_E(currentTrack_ != nullptr && currentTrack_->sampleHelper != nullptr, false,
        "Current fragment track or sampleHelper is nullptr");
    ret = currentTrack_->sampleHelper->SetTrackFragmentDecodeTimeParams(atom.dataOffset, atom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Set track fragment decode time params failed");
    return true;
}

bool MPEG4AtomParser::ProcessMoofTrun(MPEG4Atom& atom, Status& ret, int64_t& currentOffset,
    FragmentEntry& frag, int64_t sidxDts)
{
    currentOffset += static_cast<int64_t>(atom.size);
    FALSE_RETURN_V_MSG_E(currentTrack_ != nullptr && currentTrack_->sampleHelper != nullptr, false,
        "Current fragment track or sampleHelper is nullptr");
    int64_t moofDts = 0;
    int64_t moofDuration = 0;
    ret = currentTrack_->sampleHelper->SetTrackFragmentRunParams(atom.dataOffset, atom.dataSize, sidxDts,
        moofDts, moofDuration);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Set track run params failed");
    frag.firstDts = (frag.firstDts == -1) ? moofDts : frag.firstDts;
    frag.duration = (frag.duration == -1) ? moofDuration : frag.duration;
    return true;
}

bool MPEG4AtomParser::ProcessMoofSenc(MPEG4Atom& atom, Status& ret, int64_t& currentOffset)
{
    currentOffset += atom.size;
    auto savedLastTrack = lastTrack_;
    lastTrack_ = currentTrack_;
    MEDIA_LOG_I("ParseMoof senc: currentTrackId=%d, currentFragmentIndexBase=%u, "
        "sampleEncryptionEntries.size=%zu",
        currentTrack_ ? currentTrack_->trackId : -1,
        currentTrack_ ? currentTrack_->currentFragmentIndexBase : 0,
        currentTrack_ ? currentTrack_->drmInfo.sampleEncryptionEntries.size() : 0);
    ParseContext sencCtx;
    sencCtx.offset = currentOffset;
    sencCtx.dataOffset = atom.dataOffset;
    ret = ParseSenc(atom, 0, &sencCtx);
    lastTrack_ = savedLastTrack;
    return true;
}

bool MPEG4AtomParser::ProcessMoofSaiz(MPEG4Atom& atom, Status& ret, int64_t& currentOffset)
{
    currentOffset += atom.size;
    ret = ParseSaiz(atom);
    return true;
}

bool MPEG4AtomParser::ProcessMoofSaio(MPEG4Atom& atom, Status& ret, int64_t& currentOffset)
{
    currentOffset += atom.size;
    ret = ParseSaio(atom);
    return true;
}

void MPEG4AtomParser::SetFileDuration()
{
    int64_t fileDuration = 0;
    bool hasTrackStartOffset = false;
    if (!hasMoofBox_) {
        for (auto track = firstTrack_; track; track = track->next) {
            if (IsImageMimeType(track->sampleHelper ? track->sampleHelper->mimeType_ : "")) {
                continue;
            }
            int64_t trackStartTime = 0;
            if (mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_START_TIME>(trackStartTime) &&
                trackStartTime > 0) {
                hasTrackStartOffset = true;
                break;
            }
        }
    }
    bool needSidxTrackEndDuration = hasSidxBox_ && hasTrackStartOffset;
    if (!hasMoofBox_ && !needSidxTrackEndDuration &&
        mediaInfo_.general.Get<Tag::MEDIA_DURATION>(fileDuration) && fileDuration > 0) {
        return; // 已经设置了有效的duration，直接返回
    }
    bool isGltf = false;
    FALSE_RETURN_MSG_D(hasMoofBox_ || !mediaInfo_.general.Get<Tag::IS_GLTF>(isGltf) || !isGltf,
        "Skip calculated file duration for glTF without valid mvhd duration");
    if (!hasMoofBox_) {
        fileDuration = GetMpeg4FileDuration();
    } else {
        int64_t maxDuration = 0;
        uint32_t maxDurationTrackIndex = 0;
        fileDuration = FindMaxTrackDuration(maxDuration, maxDurationTrackIndex);
        int64_t containerStartTime = 0;
        if (mediaInfo_.general.Get<Tag::MEDIA_CONTAINER_START_TIME>(containerStartTime) &&
            containerStartTime > 0 && fileDuration > containerStartTime) {
            fileDuration -= containerStartTime;
        }
    }
    if (fileDuration <= 0) {
        MEDIA_LOG_D("Skip invalid file duration: " PUBLIC_LOG_D64 " us", fileDuration);
        return;
    }
    // 设置文件的最终duration
    mediaInfo_.general.Set<Tag::MEDIA_DURATION>(fileDuration);
    MEDIA_LOG_D("Set file duration: " PUBLIC_LOG_D64 " us", fileDuration);
}

int64_t MPEG4AtomParser::GetMpeg4FileDuration()
{
    double earliestStartTime = std::numeric_limits<double>::max();
    double latestEndTime = std::numeric_limits<double>::lowest();
    int32_t trackTimeScale = 0;
    int64_t trackStartTime = 0;
    double trackEndTime = 0;
    int64_t containerStartTime = 0;
    bool hasValidDuration = false;
    mediaInfo_.general.Get<Tag::MEDIA_CONTAINER_START_TIME>(containerStartTime);
    MEDIA_LOG_D("Container start time: " PUBLIC_LOG_D64 " us", containerStartTime);

    // 遍历所有轨道，找出最早开始时间和最晚结束时间
    for (auto track = firstTrack_; track; track = track->next) {
        FALSE_CONTINUE_NOLOG(!IsImageMimeType(track->sampleHelper ? track->sampleHelper->mimeType_ : ""));
        int64_t trackDuration = GetTrackDuration(track);
        mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_START_TIME>(trackStartTime);
        trackTimeScale = track->timeScale;
        FALSE_CONTINUE_NOLOG(trackDuration > 0 && trackTimeScale > 0);

        double trackStartTimeSeconds = static_cast<double>(trackStartTime) / S_TO_US;
        double trackDurationInSeconds = static_cast<double>(trackDuration) / trackTimeScale;
        hasValidDuration = true;
        // 更新最早开始时间和最晚结束时间（以秒为单位）
        if (trackStartTimeSeconds < earliestStartTime) {
            earliestStartTime = trackStartTimeSeconds;
        }

        trackEndTime = trackStartTimeSeconds + trackDurationInSeconds;
        if (trackEndTime > latestEndTime) {
            latestEndTime = trackEndTime;
        }
    }
    if (!hasValidDuration) {
        return 0;
    }
    int64_t fileDuration = static_cast<int64_t>(std::round(latestEndTime * S_TO_US));
    return fileDuration;
}

int64_t MPEG4AtomParser::GetSidxTrackStartTime(const std::shared_ptr<Track>& track, int64_t containerStartTime)
{
    FALSE_RETURN_V_NOLOG(hasSidxBox_ && containerStartTime <= 0, 0);
    int32_t timeScale = 0;
    if (track->elstInitEmptyEdit > 0) {
        int32_t fileTimeScale = movieTimeScale_;
        int64_t emptyEditInTrackTime = 0;
        timeScale = track->timeScale;
        if (fileTimeScale > 0 && timeScale > 0 &&
            ConvertTimeScaleRoundToTimeBase(track->elstInitEmptyEdit, timeScale, fileTimeScale,
                &emptyEditInTrackTime) == Status::OK) {
            int64_t trackStartTime = 0;
            (void)ConvertTrackTimeToUs(emptyEditInTrackTime, timeScale, &trackStartTime);
            return trackStartTime;
        }
        return 0;
    }
    FALSE_RETURN_V_NOLOG(track->sampleHelper != nullptr && !track->sampleHelper->IsSampleIndexEmpty() &&
        track->timeScale > 0, 0);
    timeScale = track->timeScale;
    int64_t firstPts = 0;
    FALSE_RETURN_V_NOLOG(track->sampleHelper->GetSamplePts(0, firstPts) == Status::OK, 0);
    return static_cast<int64_t>(std::round(
        static_cast<double>(firstPts) * S_TO_US / timeScale));
}

int64_t MPEG4AtomParser::GetSidxSampleEndTime(const std::shared_ptr<Track>& track, int32_t timeScale)
{
    FALSE_RETURN_V_NOLOG(hasSidxBox_ && track->sampleHelper != nullptr &&
        !track->sampleHelper->IsSampleIndexEmpty(), 0);
    int64_t sampleDtsEnd = GetMaxSampleDtsEnd(track->sampleHelper);
    FALSE_RETURN_V_NOLOG(sampleDtsEnd > 0 && timeScale > 0, 0);
    int64_t durationUs = 0;
    FALSE_RETURN_V_MSG_E(!(__builtin_mul_overflow(sampleDtsEnd, S_TO_US, &durationUs)), 0,
        "Mul overflow: [" PUBLIC_LOG_D64 "," PUBLIC_LOG_D64 "]", sampleDtsEnd, S_TO_US);
    int64_t safeTimeScale = static_cast<int64_t>(timeScale);
    return (durationUs + safeTimeScale - 1) / safeTimeScale;
}

int64_t MPEG4AtomParser::FindMaxTrackDuration(int64_t& maxDuration, uint32_t& maxDurationTrackIndex)
{
    FileType fileType = FileType::MP4;
    mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType);
    bool isRawFile = (fileType == FileType::M4A); // M4V 和 M4A文件通常标记为M4A类型
    int64_t containerStartTime = 0;
    (void)mediaInfo_.general.Get<Tag::MEDIA_CONTAINER_START_TIME>(containerStartTime);
    for (auto track = firstTrack_; track; track = track->next) {
        FALSE_CONTINUE_NOLOG(!IsImageMimeType(track->sampleHelper ? track->sampleHelper->mimeType_ : ""));
        double trackDurationRaw = CalculateDuration(track, isRawFile);
        MEDIA_LOG_D("Calculated duration for track " PUBLIC_LOG_U32 ": " PUBLIC_LOG_F " s",
            track->trackIndex, trackDurationRaw);
        if (trackDurationRaw <= 0) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has no valid sample data", track->trackIndex);
            continue;
        }
        int64_t trackDuration = static_cast<int64_t>(trackDurationRaw);
        int64_t trackStartTime = GetSidxTrackStartTime(track, containerStartTime);
        int64_t trackEndTime = 0;
        if (__builtin_add_overflow(trackStartTime, trackDuration, &trackEndTime)) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " duration overflow: start=" PUBLIC_LOG_D64 ", duration="
                PUBLIC_LOG_D64, track->trackIndex, trackStartTime, trackDuration);
            continue;
        }
        if (hasSidxBox_ && containerStartTime <= 0 && trackStartTime > 0) {
            ++trackEndTime;
        }
        int32_t timeScale = track->timeScale;
        if (hasSidxBox_ && containerStartTime <= 0 && timeScale > 0) {
            trackEndTime = std::max(trackEndTime, GetSidxSampleEndTime(track, timeScale));
        }
        if (trackEndTime > maxDuration) {
            maxDuration = trackEndTime;
            maxDurationTrackIndex = track->trackIndex;
        }
    }
    return (maxDuration > 0) ? maxDuration : 0;
}

double MPEG4AtomParser::CalculateDuration(std::shared_ptr<Track> track, bool isRawFile)
{
    FALSE_RETURN_V_MSG_E(track != nullptr, 0.0, "Track is null");
    FALSE_RETURN_V_MSG_E(track->sampleHelper != nullptr, 0.0, "SampleHelper is null");
    FALSE_RETURN_V_MSG_E(!track->sampleHelper->IsSampleIndexEmpty(), 0.0, "Sample data is empty");

    int32_t timeScale = track->timeScale;
    FALSE_RETURN_V_MSG_W(timeScale > 0, 0.0, "Track " PUBLIC_LOG_U32 " has invalid time scale", track->trackIndex);

    int64_t firstPts = 0;
    FALSE_RETURN_V_MSG_E(track->sampleHelper->GetSamplePts(0, firstPts) == Status::OK, 0.0,
        "Get first sample pts failed");
    int64_t baseDuration = 0;
    int64_t ctsTail = 0;

    if (hasSidxBox_ && track->sidxDuration > 0) {
        // FMP4文件有sidx box，使用sidx中的duration
        baseDuration = track->sidxDuration;
    } else {
        // FMP4文件没有sidx box，使用trun duration
        baseDuration = track->sampleHelper->totalTrunDuration_;
    }

    if (hasMoofBox_) {
        int64_t maxPtsEnd = GetMaxSamplePtsEnd(track->sampleHelper);
        ctsTail = maxPtsEnd > track->sampleHelper->totalTrunDuration_ ?
            maxPtsEnd - track->sampleHelper->totalTrunDuration_ : 0;
        if (!hasSidxBox_ || track->sidxDuration <= 0) {
            baseDuration = std::max(baseDuration, maxPtsEnd);
        }
    }

    // The edit-list empty duration is still exposed as start_time, but must not be added again to duration.
    int64_t totalDuration = baseDuration;
    if (!hasSidxBox_ || track->sidxDuration <= 0) {
        totalDuration += track->elstInitEmptyEdit;
    }
    int64_t durationUs = 0;
    if (__builtin_mul_overflow(totalDuration, S_TO_US, &durationUs)) {
        MEDIA_LOG_E("Mul overflow: [" PUBLIC_LOG_D64 "," PUBLIC_LOG_D64 "]", totalDuration, S_TO_US);
    }
    double duration = static_cast<double>(durationUs) / timeScale;
    duration = (hasMoofBox_ && ctsTail > 0) ? std::floor(duration) : std::round(duration);
    if (!isRawFile && !hasMoofBox_) {
        duration += static_cast<double>(firstPts) * S_TO_US / timeScale;
    }
    MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " duration: " PUBLIC_LOG_F, track->trackIndex, duration);
    return duration;
}

void MPEG4AtomParser::UpdateTrackStartTime(const std::shared_ptr<Track>& track, int64_t& trackStartTime,
    int64_t& containerStartTimeForTrack)
{
    if (mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_START_TIME>(trackStartTime)) {
        MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " already has start time", track->trackIndex);
        containerStartTimeForTrack = trackStartTime;
        return;
    }
    bool isGltf = false;
    int64_t fileDuration = 0;
    if (mediaInfo_.general.Get<Tag::IS_GLTF>(isGltf) && isGltf &&
        !mediaInfo_.general.Get<Tag::MEDIA_DURATION>(fileDuration)) {
        trackStartTime = 0;
        mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_START_TIME>(trackStartTime);
        containerStartTimeForTrack = trackStartTime;
        return;
    }
    if (track->elstInitEmptyEdit > 0) {
        int32_t fileTimeScale = movieTimeScale_;
        int32_t trackTimeScale = track->timeScale;
        if (fileTimeScale > 0 && trackTimeScale > 0) {
            int64_t emptyEditInTrackTime = 0;
            FALSE_RETURN_MSG(ConvertTimeScaleRoundToTimeBase(track->elstInitEmptyEdit, trackTimeScale,
                fileTimeScale, &emptyEditInTrackTime) == Status::OK, "Mul overflow");
            FALSE_RETURN_MSG(ConvertTrackTimeToUs(emptyEditInTrackTime, trackTimeScale,
                &trackStartTime) == Status::OK, "Mul overflow");
            int64_t containerStartTime = 0;
            FALSE_RETURN_MSG(ConvertTimeScaleRound(emptyEditInTrackTime, S_TO_US,
                trackTimeScale, &containerStartTime) == Status::OK, "Mul overflow");
            mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_START_TIME>(trackStartTime);
            containerStartTimeForTrack = containerStartTime;
            return;
        }
    } else if (track->sampleHelper && !track->sampleHelper->IsSampleIndexEmpty()) {
        int64_t firstPts = 0;
        FALSE_RETURN_MSG(track->sampleHelper->GetSamplePts(0, firstPts) == Status::OK,
            "Get first sample pts failed");
        int32_t timeScale = track->timeScale;
        if (timeScale > 0) {
            FALSE_RETURN_MSG(ConvertTimeScaleRound(firstPts, S_TO_US, timeScale, &trackStartTime) == Status::OK,
                "Mul overflow");
        }
        trackStartTime = std::max<int64_t>(trackStartTime, 0);
        mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_START_TIME>(trackStartTime);
        containerStartTimeForTrack = trackStartTime;
        return;
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_START_TIME>(trackStartTime);
    containerStartTimeForTrack = trackStartTime;
}

void MPEG4AtomParser::SetStartTime()
{
    int64_t minStartTime = INT64_MAX;  // 改为64位
    int64_t minContainerStartTime = INT64_MAX;
    int64_t trackStartTime = 0;
    bool hasValidStartTime = false;
    for (auto track = firstTrack_; track; track = track->next) {
        int64_t containerStartTimeForTrack = INT64_MAX;
        trackStartTime = 0;
        if (IsImageMimeType(track->sampleHelper ? track->sampleHelper->mimeType_ : "")) {
            continue;
        }
        UpdateTrackStartTime(track, trackStartTime, containerStartTimeForTrack);
        if (trackStartTime < minStartTime) {
            minStartTime = trackStartTime;
            hasValidStartTime = true;
        }
        if (containerStartTimeForTrack != INT64_MAX) {
            minContainerStartTime = std::min(minContainerStartTime, containerStartTimeForTrack);
        } else {
            minContainerStartTime = std::min(minContainerStartTime, trackStartTime);
        }
    }
    int64_t containerStartTime = (hasValidStartTime && minContainerStartTime != INT64_MAX) ? minContainerStartTime : 0;
    mediaInfo_.general.Set<Tag::MEDIA_CONTAINER_START_TIME>(containerStartTime);
}

void MPEG4AtomParser::SetHEVCDefaultValue()
{
    ColorPrimary colorPrimaries = ColorPrimary::UNSPECIFIED;
    TransferCharacteristic colorTrc = TransferCharacteristic::UNSPECIFIED;
    MatrixCoefficient colorMatrix = MatrixCoefficient::UNSPECIFIED;
    ChromaLocation chromaLocation = ChromaLocation::UNSPECIFIED;
    for (auto track = firstTrack_; track; track = track->next) {
        if (track->sampleHelper->mimeType_ == MimeType::VIDEO_HEVC) {
            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries)) {
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_RANGE>(false);
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_PRIMARIES>(ColorPrimary::UNSPECIFIED);
            }

            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_COLOR_TRC>(colorTrc)) {
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_TRC>(TransferCharacteristic::UNSPECIFIED);
            }

            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix)) {
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(MatrixCoefficient::UNSPECIFIED);
            }
            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_CHROMA_LOCATION>(chromaLocation)) {
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_CHROMA_LOCATION>(ChromaLocation::UNSPECIFIED);
            }
        }
    }
}

MediaInfo MPEG4AtomParser::GetMediaInfo() const
{
    return mediaInfo_;
}

std::shared_ptr<MPEG4AtomParser::Track> MPEG4AtomParser::GetFirstTrack() const
{
    return firstTrack_;
}

std::shared_ptr<Meta> MPEG4AtomParser::GetUserFormat() const
{
    return userFormat_;
}

void MPEG4AtomParser::GetFragmentEntry(std::vector<FragmentEntry>& entry)
{
    fragmentEntry_.swap(entry);
}

void MPEG4AtomParser::SetDataReader(const std::shared_ptr<DemuxerDataReader> dataReader)
{
    dataReader_ = dataReader;
}

void MPEG4AtomParser::SetSeekable(const Seekable seekable)
{
    seekable_ = seekable;
}

bool MPEG4AtomParser::GetHasSidx()
{
    return hasSidxBox_;
}

bool MPEG4AtomParser::GetHasMoof()
{
    return hasMoofBox_;
}

int32_t MPEG4AtomParser::GetMovieTimeScale() const
{
    return movieTimeScale_;
}

constexpr std::array<std::string_view, 139> mdhd_language_map = {{
    "eng",    /* 0 English */         "fra",    /* 1 French */          "ger",    /* 2 German */
    "ita",    /* 3 Italian */         "dut",    /* 4 Dutch */           "sve",    /* 5 Swedish */
    "spa",    /* 6 Spanish */         "dan",    /* 7 Danish */          "por",    /* 8 Portuguese */
    "nor",    /* 9 Norwegian */       "heb",    /* 10 Hebrew */         "jpn",    /* 11 Japanese */
    "ara",    /* 12 Arabic */         "fin",    /* 13 Finnish */        "gre",    /* 14 Greek */
    "ice",    /* 15 Icelandic */      "mlt",    /* 16 Maltese */        "tur",    /* 17 Turkish */
    "hr ",    /* 18 Croatian */       "chi",    /* 19 Traditional Chinese */ "urd", /* 20 Urdu */
    "hin",    /* 21 Hindi */          "tha",    /* 22 Thai */           "kor",    /* 23 Korean */
    "lit",    /* 24 Lithuanian */     "pol",    /* 25 Polish */         "hun",    /* 26 Hungarian */
    "est",    /* 27 Estonian */       "lav",    /* 28 Latvian */        "smi",    /* 29 Sami */
    "fo ",    /* 30 Faroese */        "per",    /* 31 Farsi */          "rus",    /* 32 Russian */
    "chi",    /* 33 Simplified Chinese */ "",  /* 34 Flemish */         "iri",    /* 35 Irish */
    "alb",    /* 36 Albanian */       "ron",    /* 37 Romanian */       "ces",    /* 38 Czech */
    "slk",    /* 39 Slovak */         "slv",    /* 40 Slovenian */      "yid",    /* 41 Yiddish */
    "sr ",    /* 42 Serbian */        "mac",    /* 43 Macedonian */     "bul",    /* 44 Bulgarian */
    "ukr",    /* 45 Ukrainian */      "bel",    /* 46 Belarusian */     "uzb",    /* 47 Uzbek */
    "kaz",    /* 48 Kazakh */         "aze",    /* 49 Azerbaijani */    "aze",    /* 50 AzerbaijanAr */
    "arm",    /* 51 Armenian */       "geo",    /* 52 Georgian */       "mol",    /* 53 Moldavian */
    "kir",    /* 54 Kirghiz */        "tgk",    /* 55 Tajiki */         "tuk",    /* 56 Turkmen */
    "mon",    /* 57 Mongolian */      "",       /* 58 MongolianCyr */   "pus",    /* 59 Pashto */
    "kur",    /* 60 Kurdish */        "kas",    /* 61 Kashmiri */       "snd",    /* 62 Sindhi */
    "tib",    /* 63 Tibetan */        "nep",    /* 64 Nepali */         "san",    /* 65 Sanskrit */
    "mar",    /* 66 Marathi */        "ben",    /* 67 Bengali */        "asm",    /* 68 Assamese */
    "guj",    /* 69 Gujarati */       "pa ",    /* 70 Punjabi */        "ori",    /* 71 Oriya */
    "mal",    /* 72 Malayalam */      "kan",    /* 73 Kannada */        "tam",    /* 74 Tamil */
    "tel",    /* 75 Telugu */         "sin",    /* 76 Sinhala */        "bur",    /* 77 Burmese */
    "khm",    /* 78 Khmer */          "lao",    /* 79 Lao */            "vie",    /* 80 Vietnamese */
    "ind",    /* 81 Indonesian */     "tgl",    /* 82 Tagalog */        "may",    /* 83 MalayRoman */
    "may",    /* 84 MalayArabic */    "amh",    /* 85 Amharic */        "tir",    /* 86 Galla */
    "orm",    /* 87 Oromo */          "som",    /* 88 Somali */         "swa",    /* 89 Swahili */
    "kin",    /* 90 Kinyarwanda */    "run",    /* 91 Rundi */          "nya",    /* 92 Nyanja */
    "mlg",    /* 93 Malagasy */       "epo",    /* 94 Esperanto */      "",       /* 95 */
    "",       /* 96 */                "",       /* 97 */                "",       /* 98 */
    "",       /* 99 */                "",       /* 100 */               "",       /* 101 */
    "",       /* 102 */               "",       /* 103 */               "",       /* 104 */
    "",       /* 105 */               "",       /* 106 */               "",       /* 107 */
    "",       /* 108 */               "",       /* 109 */               "",       /* 110 */
    "",       /* 111 */               "",       /* 112 */               "",       /* 113 */
    "",       /* 114 */               "",       /* 115 */               "",       /* 116 */
    "",       /* 117 */               "",       /* 118 */               "",       /* 119 */
    "",       /* 120 */               "",       /* 121 */               "",       /* 122 */
    "",       /* 123 */               "",       /* 124 */               "",       /* 125 */
    "",       /* 126 */               "",       /* 127 */               "wel",    /* 128 Welsh */
    "baq",    /* 129 Basque */        "cat",    /* 130 Catalan */       "lat",    /* 131 Latin */
    "que",    /* 132 Quechua */       "grn",    /* 133 Guarani */       "aym",    /* 134 Aymara */
    "tat",    /* 135 Tatar */         "uig",    /* 136 Uighur */        "dzo",    /* 137 Dzongkha */
    "jav",    /* 138 JavaneseRom */
}};

std::string MPEG4AtomParser::ConvertLanguageCode(uint16_t langcode)
{
    constexpr int32_t stringLength = 4;
    if (langcode >= 0x0400 && langcode != 0x7FFF) {
        constexpr int16_t langLength = 3;
        std::array<char, stringLength> language{};
        uint16_t code = langcode;
        // 从低位到高位提取3个5位字段
        for (int32_t i = 2; i >= 0; i--) {
            language[i] = static_cast<char>(0x60 + (code & 0x1F));
            code >>= 0x05;
        }
        return std::string(language.data(), langLength);
    }
    if (langcode < mdhd_language_map.size()) {
        std::string_view lang_view = mdhd_language_map[langcode];
        if (!lang_view.empty()) {
            std::string result(lang_view);
            result.erase(std::find_if(result.rbegin(), result.rend(),
                [](uint8_t ch) { return !std::isspace(ch); }).base(), result.end());
            return result;
        }
    }
    return "";
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
