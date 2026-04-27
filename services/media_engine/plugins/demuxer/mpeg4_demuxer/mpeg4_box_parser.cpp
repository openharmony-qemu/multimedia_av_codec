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
#include <unistd.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <limits>
#include <regex>
#include <set>
#include <arpa/inet.h>
#include "avcodec_trace.h"
#include "securec.h"
#include "mpeg4_box_parser.h"
#include "mpeg4_utils.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "meta/video_types.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4BoxParser" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
constexpr uint64_t MIN_FLOAT_SIZE = 4;
constexpr uint32_t ALAC_SPECIFIC_SIZE = 36;
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
constexpr int64_t HDR_VIVID_TAG_LENGTH = 31;
constexpr int64_t HDR_VIVID_TAG_OFFSET = 43;
constexpr int32_t MATRIX_SIZE = 4;
constexpr int32_t MAX_DEPTH = 10;
constexpr uint64_t MAX_TITLE_SIZE = 24;
constexpr uint64_t MAX_EXTRA_DATA_SIZE = 1024 * 1024;
constexpr uint64_t MAX_DATA_SIZE = 256;
constexpr uint64_t MIN_COLOR_SIZE = 4;
constexpr uint64_t MIN_COLOR_PARAM_SIZE = 6;
constexpr uint64_t MIN_MVHD_SIZE = 32;
constexpr uint64_t MIN_TKHD_SIZE = 84;
constexpr uint64_t MIN_HDLR_DATA_SIZE = 24;
constexpr uint64_t MIN_BOX_SIZE = 16;
constexpr uint64_t MIN_TFHD_SIZE = 8;
constexpr uint64_t MIN_VIDEO_SAMPLE_ENTRY_SIZE = 78;
constexpr uint64_t MIN_AUDIO_SAMPLE_ENTRY_SIZE = 28;
constexpr uint64_t MIN_ACLR_SIZE = 13;
constexpr uint64_t MIN_PCMC_SIZE = 6;
constexpr uint64_t MIN_SIDX_SIZE = 12;
constexpr uint64_t MIN_TIMED_METADATA_SAMPLE_ENTRY_SIZE = 16;
constexpr uint64_t MIN_TRADITIONAL_METADATA_SIZE = 4;
constexpr uint64_t MIN_LOCI_SIZE = 20;
constexpr uint64_t MIN_CLLI_SIZE = 12;
constexpr uint64_t MIN_MDCV_SIZE = 32;
constexpr int32_t MPEG4_ROOT_DEPTH = 0;  // Root level atom, ftyp, moov, mdat etc.
constexpr int32_t MPEG4_DEPTH_ONE = 1;  // moov's children atom, mvhd, trak, mvex etc.
constexpr int32_t MPEG4_DEPTH_TWO = 2;  // trak's direct children atom, tkhd, mdia, edts etc.
constexpr int32_t MPEG4_DEPTH_THREE = 3;  // mdia's direct children atom, mdhd, hdlr, minf etc.
constexpr int32_t MPEG4_DEPTH_FOUR = 4;  // minf's direct children atom, vmhd, smhd, stbl etc.
constexpr uint16_t G711MULAW_SAMPLE_SIZE = 8;
constexpr uint8_t ID3V1_GENRE_MAX = 191;
constexpr uint64_t QUICKTIME_EPOCH_OFFSET = 2082844800;
constexpr size_t OFFSET_V0 = 12;
constexpr size_t OFFSET_V1 = 20;
constexpr size_t OPUS_MIN_SIZE = 11;
constexpr size_t MAX_SIZE = 1 << 30;
constexpr size_t DURATION_OFFSET_V0 = 16;
constexpr size_t DURATION_OFFSET_V1 = 24;
constexpr size_t DIRECT_META_PATH_SIZE = 2;  // moov/meta 直接结构的路径大小
constexpr uint8_t AOT_SBR = 5;
constexpr uint8_t AOT_PS = 29;
constexpr uint8_t AAC_MAIN = 1;
constexpr uint8_t AAC_LC = 2;
constexpr uint8_t AAC_SSR = 3;
constexpr uint8_t AAC_LTP = 4;
constexpr uint8_t AAC_ER_LC = 17;
constexpr uint8_t AAC_ER_LD = 23;
constexpr uint8_t AAC_ER_ELD = 39;
// ESDS 描述符标签常量
constexpr uint8_t MPEG4_ES_DESCR_TAG = 0x03;
constexpr uint8_t MPEG4_DEC_CONFIG_DESCR_TAG = 0x04;
constexpr uint8_t MPEG4_DEC_SPECIFIC_DESCR_TAG = 0x05;
constexpr float LUMI_SCALE = 0.0001;
constexpr float PRIMARY_SCALE = 0.00002;
constexpr uint64_t CREATION_TIME_SCALE = 1000000;

constexpr std::array<size_t, 4> MATRIX_2X2_INDICES = {0, 1, 3, 4}; // [0][0], [0][1], [1][0], [1][1]
constexpr std::array<size_t, 3> MATRIX_DIAGONAL_INDICES = {0, 4, 8}; // [0][0], [1][1], [2][2]
constexpr std::array<size_t, 6> MATRIX_OFF_DIAGONAL_INDICES = {1, 2, 3, 5, 6, 7}; // 非对角线元素

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

static std::string GetMimeType(int32_t atomtype)
{
    static std::map<int32_t, std::string> typeToMime = {
        { FourccType("mp4v"), MimeType::VIDEO_MPEG4 },
        { FourccType("s263"), MimeType::VIDEO_H263 },
        { FourccType("H263"), MimeType::VIDEO_H263 },
        { FourccType("h263"), MimeType::VIDEO_H263 },
        { FourccType("avc1"), MimeType::VIDEO_AVC },
        { FourccType("acv3"), MimeType::VIDEO_AVC },
        { FourccType("hvc1"), MimeType::VIDEO_HEVC },
        { FourccType("hev1"), MimeType::VIDEO_HEVC },
        { FourccType("vvc1"), MimeType::VIDEO_VVC },
        { FourccType("vvi1"), MimeType::VIDEO_VVC },
        { FourccType("fLaC"), MimeType::AUDIO_FLAC },
        { FourccType(".mp2"), MimeType::AUDIO_MPEG },
        { FourccType(".mp3"), MimeType::AUDIO_MPEG },
        { 0x6D730055,         MimeType::AUDIO_MPEG },
        { FourccType("mp4a"), MimeType::AUDIO_AAC },
        { FourccType("av3a"), MimeType::AUDIO_AVS3DA },
        { FourccType("alaw"), MimeType::AUDIO_G711A },
        { FourccType("ulaw"), MimeType::AUDIO_G711MU },
        { FourccType("fl32"), MimeType::AUDIO_RAW },
        { FourccType("fl64"), MimeType::AUDIO_RAW },
        { FourccType("in24"), MimeType::AUDIO_RAW },
        { FourccType("in32"), MimeType::AUDIO_RAW },
        { FourccType("raw "), MimeType::AUDIO_RAW },
        { FourccType("NONE"), MimeType::AUDIO_RAW },
        { FourccType("ipcm"), MimeType::AUDIO_RAW },
        { FourccType("lpcm"), MimeType::AUDIO_RAW },
        { FourccType("twos"), MimeType::AUDIO_RAW },
        { FourccType("sowt"), MimeType::AUDIO_RAW },
        { FourccType("msVo"), MimeType::AUDIO_VORBIS },
        { FourccType("Opus"), MimeType::AUDIO_OPUS },
        { FourccType("samr"), MimeType::AUDIO_AMR_NB },
        { FourccType("sawb"), MimeType::AUDIO_AMR_WB },
        { FourccType("alac"), "audio/alac"},
        { FourccType("dtsc"), "audio/dts" },
        { FourccType("dtsh"), "audio/dts" },
        { FourccType("dtsl"), "audio/dts" },
        { FourccType("dtse"), "audio/dts" },
        { FourccType("wvtt"), MimeType::TEXT_WEBVTT },
        { FourccType("mebx"), MimeType::TIMED_METADATA },
    };

    auto it = typeToMime.find(atomtype);
    if (it != typeToMime.end()) {
        return it->second;
    }
    return MimeType::INVALID_TYPE;
}

static bool UserMetaToSource(std::string key)
{
    static std::vector<std::string> keys = {
        "aigc"
    };
    auto it = std::find(keys.begin(), keys.end(), key);
    return it != keys.end();
}

static bool StartWith(const char* name, const char* chars)
{
    if (name == nullptr || chars == nullptr) {
        return false;
    }
    return !strncmp(name, chars, strlen(chars));
}


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

static const std::map<int32_t, AudioChannelLayout> FLAC_CHANNEL_LAYOUT_TABLE = {
    {1, AudioChannelLayout::MONO},
    {2, AudioChannelLayout::STEREO},
    {3, AudioChannelLayout::SURROUND},
    {4, AudioChannelLayout::QUAD},
    {5, AudioChannelLayout::CH_5POINT0},
    {6, AudioChannelLayout::CH_5POINT1},
    {7, AudioChannelLayout::CH_6POINT1},
    {8, AudioChannelLayout::CH_7POINT1}
};

static const std::map<int32_t, AudioChannelLayout> ALAC_CHANNEL_LAYOUT_TABLE = {
    {1, AudioChannelLayout::MONO},
    {2, AudioChannelLayout::STEREO},
    {3, AudioChannelLayout::SURROUND},
    {4, AudioChannelLayout::CH_4POINT0},
    {5, AudioChannelLayout::CH_5POINT0_BACK},
    {6, AudioChannelLayout::CH_5POINT1_BACK},
    {7, AudioChannelLayout::CH_6POINT1_BACK},
    {8, AudioChannelLayout::CH_7POINT1_WIDE_BACK},
    {9, AudioChannelLayout::UNKNOWN}
};

static const std::map<int64_t, AudioChannelLayout> PCM_CHANNEL_LAYOUT_TABLE = {
    {(100 << 16) | 1, AudioChannelLayout::MONO},
    {(101 << 16) | 2, AudioChannelLayout::STEREO},
    {(113 << 16) | 3, AudioChannelLayout::SURROUND},
    {(108 << 16) | 4, AudioChannelLayout::QUAD},
    {(117 << 16) | 5, AudioChannelLayout::CH_5POINT0},
    {(121 << 16) | 6, AudioChannelLayout::CH_5POINT1},
    {(125 << 16) | 7, AudioChannelLayout::CH_6POINT1},
    {(126 << 16) | 8, AudioChannelLayout::CH_7POINT1_WIDE},
    {(128 << 16) | 8, AudioChannelLayout::CH_7POINT1},
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

enum class FieldOrder {
    FIELD_ORDER_UNKNOWN,
    FIELD_ORDER_PROGRESSIVE,
    FIELD_ORDER_TT,  // top coded first, top displayed first
    FIELD_ORDER_BB,  // bottom coded first, bottom displayed first
    FIELD_ORDER_TB,  // top coded first, bottom displayed first
    FIELD_ORDER_BT,  // bottom coded first, top displayed first
};

enum AACDataBlockType : uint8_t {
    TYPE_SCE,
    TYPE_CPE,
    TYPE_CCE,
    TYPE_LFE,
    TYPE_DSE,
    TYPE_PCE,
    TYPE_FIL,
    TYPE_END
};

MPEG4AtomParser::MPEG4AtomParser()
    : firstTrack_(nullptr),
      lastTrack_(nullptr),
      isIsom_(false),
      hasMdatBox_(false),
      hasSidxBox_(false),
      isDistributedSidx_(false),
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
        if (matrix[idx] != 0) {
            return false;
        }
    }
    return true;
}

bool IsDisplayMatrixIdentity(const int32_t matrix[3][3])
{
    return IsIdentityMatrix(reinterpret_cast<const int32_t*>(matrix));
}

void MultiplyDisplayMatrix(const int32_t a[3][3], const int32_t b[3][3], int32_t result[3][3])
{
    FALSE_RETURN_MSG(a != nullptr && b != nullptr && result != nullptr, "Invalid params");
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

static int64_t ReadChannelSet(uint32_t tag)
{
    const uint32_t chanTopBackRightTag = 18;
    const uint32_t chanWideLeftTag = 35;
    const uint32_t chanWideRightTag = 36;
    const uint32_t chanLowFrequency2 = 37;
    const uint32_t chanStereoLeft = 38;
    const uint32_t chanStereoRight = 39;

    if (tag > 0 && tag <= chanTopBackRightTag) {
        return 1U << (tag - 1);
    }

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
    if (tag == 0) {
        return 0;
    } else if (tag == (1 << 0x10)) {
        return bitmap < 0x040000 ? bitmap : 0;
    }

    constexpr uint32_t maxChannels = 9;
    uint32_t channels = tag & 0xFFFF;
    if (channels > maxChannels) {
        return 0;
    }
    std::map<int64_t, AudioChannelLayout> layoutMap = {};
    std::string mime = lastTrack_->sampleHelper->mimeType_.c_str();
    if (mime == MimeType::AUDIO_RAW) {
        layoutMap = PCM_CHANNEL_LAYOUT_TABLE;
    }

    auto it = layoutMap.find(tag);
    if (it != layoutMap.end()) {
        return static_cast<int64_t>(it->second);
    } else {
        return 0;
    }
}

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

    if (scale[0] == 0.0 || scale[1] == 0.0) {
        MEDIA_LOG_W("Invalid matrix: zero scale factor");
        return std::numeric_limits<double>::quiet_NaN();
    }

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
        Status ret = MPEG4ParseAtom(depth + 1, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse atom at offset " PUBLIC_LOG_D64, ctx->offset);
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA,
        "Finished parsing container atom at offset " PUBLIC_LOG_D64 ", expected end at " PUBLIC_LOG_D64,
        ctx->offset, endOffset);

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

    uint32_t compatibleBrandsCount = (currentAtom.dataSize - 8) / 4;
    FALSE_RETURN_V_MSG_E(compatibleBrandsCount > 0, Status::ERROR_INVALID_DATA, "Invalid compatible brands number");
    // 一次性读取所有compatible brands数据
    if (compatibleBrandsCount > 0) {
        for (uint32_t i = 0; i < compatibleBrandsCount; ++i) {
            // 解析和存储所有brands, 跳过 minor version，从第8字节开始
            uint32_t compatibleBrand = GetU32Value(&ftypInfo[8 + 4 * i]);
            std::string brandStr = FourccToString(compatibleBrand);
            ctx->compatibleBrands.insert(compatibleBrand);
        }
    }
    mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::MP4);
    if (StartWith(majorBrand.c_str(), "m4a") || StartWith(majorBrand.c_str(), "M4A")) {
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::M4A);
    } else if (StartWith(majorBrand.c_str(), "m4v") || StartWith(majorBrand.c_str(), "M4V")) {
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::M4V);
    } else if (StartWith(majorBrand.c_str(), "3g2")) {
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::FT_3G2);
    } else if (StartWith(majorBrand.c_str(), "3gp")) {
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::FT_3GP);
    } else if (StartWith(majorBrand.c_str(), "qt") || StartWith(majorBrand.c_str(), "QT")) {
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::MOV);
    } else {
        isIsom_ = true; // 若为MPEG4格式，设置isom标志
    }
    ctx->offset += currentAtom.size;
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
    FALSE_RETURN_V_MSG_E(currentAtom.size > 0, Status::ERROR_INVALID_DATA, "mdat atom size is zero, wrong MPEG4 file");
    hasMdatBox_ = true;
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseWide(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    if (currentAtom.dataSize < DEFAULT_HEAD_SIZE) {
        ctx->offset += currentAtom.size;
        return Status::OK;  // continue
    }

    uint8_t headBuffer[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headBuffer, sizeof(headBuffer));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read embedded size failed");

    uint32_t embeddedSize = GetU32Value(headBuffer);
    // 如果不是0 sized mdat atom，跳过剩余数据
    if (embeddedSize != 0) {
        ctx->offset += currentAtom.size;
        return Status::OK;
    }

    uint32_t embeddedType = GetU32Value(&headBuffer[4]);
    // 如果不是mdat类型，跳过剩余数据
    if (embeddedType != FourccType("mdat")) {
        ctx->offset += currentAtom.size;
        return Status::OK;
    }

    MPEG4Atom mdatAtom;
    mdatAtom.type = embeddedType;
    mdatAtom.size = currentAtom.dataSize - sizeof(headBuffer);
    mdatAtom.dataSize = mdatAtom.size;
    // 更新ctx指向mdat数据的开始位置
    int64_t originalOffset = ctx->offset;
    int64_t originalDataOffset = ctx->dataOffset;
    ctx->dataOffset = originalDataOffset + sizeof(headBuffer);

    ret = ParseMdat(mdatAtom, depth, ctx);
    ctx->dataOffset = originalDataOffset;
    ctx->offset = originalOffset + currentAtom.size;
    return ret;
}

std::string FormatTimestamp(uint64_t creationTime)
{
    time_t seconds = creationTime / CREATION_TIME_SCALE;
    struct tm* timeinfo = gmtime(&seconds);
    if (timeinfo == nullptr) {
        return "";
    }
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

Status MPEG4AtomParser::ParseMvhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_ONE, Status::ERROR_INVALID_DATA, "Invalid mvhd depth");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_MVHD_SIZE && currentAtom.dataSize <= MAX_DATA_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid atom size");
    ctx->offset += currentAtom.size;

    uint8_t headerInfo[currentAtom.dataSize];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo, sizeof(headerInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read mvhd header info failed");
    uint8_t version = headerInfo[0];
    uint64_t creationTime;
    uint64_t duration;
    uint32_t timeScale;
    int32_t matrixOffset = version == 1 ? DYNAMIC_MATRIX_OFFSET_V1 : DYNAMIC_MATRIX_OFFSET_V0;

    if (version == 1) {
        creationTime = GetU64Value(&headerInfo[CREATION_TIME_OFFSET]);
        timeScale = GetU32Value(&headerInfo[OFFSET_V1]);
        duration = GetU64Value(&headerInfo[DURATION_OFFSET_V1]);
    } else {
        creationTime = static_cast<uint64_t>(GetU32Value(&headerInfo[CREATION_TIME_OFFSET]));
        if (creationTime > 0 && creationTime < QUICKTIME_EPOCH_OFFSET) {
            creationTime += QUICKTIME_EPOCH_OFFSET; // Convert to Unix epoch time
        }
        timeScale = GetU32Value(&headerInfo[OFFSET_V0]);
        duration = static_cast<uint64_t>(GetU32Value(&headerInfo[DURATION_OFFSET_V0]));
    }
    if (timeScale <= 0) {
        timeScale = 1; // set to default value
    }
    std::string creationTimeStr;
    if (creationTime) {
        creationTime -= QUICKTIME_EPOCH_OFFSET;
        creationTimeStr = FormatTimestamp(creationTime * CREATION_TIME_SCALE);
        mediaInfo_.general.Set<Tag::MEDIA_CREATION_TIME>(creationTimeStr);
    }
    mediaInfo_.general.Set<Tag::MEDIA_TIME_SCALE>(static_cast<int32_t>(timeScale));

    int64_t durationUs = 0;
    if (ConvertTimeScale(duration, S_TO_US, timeScale, &durationUs) != Status::OK) {
        MEDIA_LOG_E("Mul overflow");
    }
    mediaInfo_.general.Set<Tag::MEDIA_DURATION>(durationUs);
    // 解析movie level display matrix
    if (currentAtom.dataSize >= static_cast<uint64_t>(matrixOffset + DISPLAY_MATRIX_SIZE)) {
        ret = ParseDisplayMatrix(&headerInfo[matrixOffset], ctx->movieDisplayMatrix);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse movie display matrix");
        ctx->hasMovieDisplayMatrix = !IsDisplayMatrixIdentity(ctx->movieDisplayMatrix); // 单位阵不是有效的旋转矩阵
        if (ctx->hasMovieDisplayMatrix) {
            MEDIA_LOG_D("Movie has non-identity display matrix");
        }
    }
    return ret;
}

int64_t MPEG4AtomParser::GetElstDuration(std::shared_ptr<Track> track)
{
    int32_t fileTimescale = 0;
    if (!mediaInfo_.general.Get<Tag::MEDIA_TIME_SCALE>(fileTimescale)) {
        MEDIA_LOG_E("Invalid file time scale");
        return 0;
    }
    int32_t trackTimescale = 0;
    if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(trackTimescale)) {
        MEDIA_LOG_E("Invalid track time scale");
        return 0;
    }
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
    if (!track || !track->sampleHelper || !IsValidTrackIndex(mediaInfo_, track->trackIndex)) {
        MEDIA_LOG_E("Invalid track for duration calculation");
        return 0;
    }
    
    int64_t duration = 0;
    if (track->sampleHelper && !track->sampleHelper->sampleIndexEntry_.empty() && !hasMoofBox_) {
        // 如果是用于比特率计算，直接返回原始的 totalDuration_
        if (calculateBitrate) {
            duration = track->sampleHelper->totalDuration_;
            MEDIA_LOG_D("Using raw sample helper duration for bitrate: " PUBLIC_LOG_D64, duration);
            return duration;
        }
        duration = std::min(track->duration, track->sampleHelper->totalDuration_);
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

void MPEG4AtomParser::CalculateTrackBitrates(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr || track->sampleHelper != nullptr, "Invalid track or sample helper");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");
    // get track time scale and duration
    int32_t timeScale = 0;
    if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale) || timeScale <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has invalid time scale", track->trackIndex);
    }

    int64_t duration = GetTrackDuration(track, true);
    const auto& samples = track->sampleHelper->sampleIndexEntry_;
    // calculate total sample size
    int64_t totalSize = 0;
    for (const auto& sample : samples) {
        totalSize += sample.size;
    }
    // bitrate = (总字节数 * 8) / 时长（秒）
    int64_t bitrate = 0;
    int64_t temp1 = 0;
    int64_t temp2 = 0;
    if (duration > 0) {
        if (!hasMoofBox_) {
            if (__builtin_mul_overflow(totalSize, BYTE_TO_BITS, &temp1) ||
                __builtin_mul_overflow(temp1, timeScale, &temp2)) {
                MEDIA_LOG_E("Mul overflow: [" PUBLIC_LOG_D64 "," PUBLIC_LOG_D32 "," PUBLIC_LOG_D32 "]",
                    totalSize, BYTE_TO_BITS, timeScale);
            }
            bitrate = temp2 / duration;
        } else {
            if (__builtin_mul_overflow(BYTE_TO_BITS, timeScale, &temp1)) {
                MEDIA_LOG_E("Mul overflow: [" PUBLIC_LOG_D32 "," PUBLIC_LOG_D32 "]",
                    BYTE_TO_BITS, timeScale);
            }
            double exactBitrate = static_cast<double>(totalSize) * temp1 / duration;
            bitrate = static_cast<int64_t>(std::round(exactBitrate));  // 四舍五入
        }
    }
    mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_BITRATE>(bitrate);
}

void MPEG4AtomParser::CalculateVideoFrameRate(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr && track->sampleHelper != nullptr, "Invalid track or sample helper");
    FALSE_RETURN_MSG(IsValidTrackIndex(mediaInfo_, track->trackIndex), "Invalid track index");
    // get track time scale and duration
    int32_t timeScale = 0;
    if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale) || timeScale <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has invalid time scale", track->trackIndex);
    }

    int64_t duration = GetTrackDuration(track);
    int64_t totalSamples = track->sampleHelper->sampleIndexEntry_.size();
    if (totalSamples <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has invalid sample count", track->trackIndex);
        return;
    }
    // 计算原始帧率
    if (duration > 0) {
        double rawFrameRate = static_cast<double>(timeScale) * totalSamples / duration;
        mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_FRAME_RATE>(rawFrameRate);
        MEDIA_LOG_D("Track:" PUBLIC_LOG_U32 ", totalSamples=" PUBLIC_LOG_D64 ", duration=" PUBLIC_LOG_D64
            ", timeScale=" PUBLIC_LOG_D32 ", frameRate=" PUBLIC_LOG_F " fps", track->trackIndex, totalSamples,
            duration, timeScale, rawFrameRate);
    }
}
void MPEG4AtomParser::SetCodecConfig(std::shared_ptr<Track> track)
{
    FALSE_RETURN_MSG(track != nullptr, "track is null");
    if (track->codecParms.extradataSize > 0 && track->codecParms.data != nullptr) {
        std::vector<uint8_t> extra(track->codecParms.extradataSize);
        extra.assign(track->codecParms.data.get(), track->codecParms.data.get() + track->codecParms.extradataSize);
        mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_CODEC_CONFIG>(extra);
    } else {
        MEDIA_LOG_E("Set codec config failed");
    }
}

Status MPEG4AtomParser::ParseTrak(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_ONE, Status::ERROR_INVALID_DATA, "Invalid trak depth");
    auto track = std::make_shared<Track>();
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate memory for track");
    
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
    // 使用通用容器解析函数
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to parse trak atom");

    if (lastTrack_->codecParms.trackType == VIDEO_TYPE) {
        ParseOrientationFromMatrix(lastTrack_);
        ParseRotationTypeFromMatrix(lastTrack_);
        CalculateSARFromTrackInfo(lastTrack_);
    }

    if (lastTrack_->sampleHelper) {
        if (lastTrack_->sampleHelper->BuildSampleIndexEntries() != Status::OK) {
            MEDIA_LOG_E("BuildSampleIndexEntry failed");
        }
    }
    SetCodecConfig(lastTrack_);
    return Status::OK;
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
        uint64_t numerator = static_cast<uint64_t>(track->stsdHeight) * track->tkhdWidth;
        uint64_t denominator = static_cast<uint64_t>(track->stsdWidth) * track->tkhdHeight;
        if (denominator > 0) {
            sar = static_cast<double>(numerator) / static_cast<double>(denominator);
            MEDIA_LOG_D("SAR from width/height diff: [%{public}u/%{public}u/%{public}u/%{public}u], sar=%{public}f",
                track->stsdWidth, track->stsdHeight, track->tkhdWidth, track->tkhdHeight, sar);
            mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_SAR>(sar);
            return;
        }
    }
    
    MEDIA_LOG_D("Track %{public}u SAR: no pasp, no width/height diff", track->trackIndex);
}

void MPEG4AtomParser::SetTrackDisplayMatrix(std::shared_ptr<Track> track, const int32_t resultMatrix[3][3],
    uint32_t width, uint32_t height)
{
    if (!IsDisplayMatrixIdentity(resultMatrix)) {
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
    ctx->offset += currentAtom.size;
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
            MultiplyDisplayMatrix(trackDisplayMatrix, ctx->movieDisplayMatrix, resultMatrix);
        } else {
            if (memcpy_s(&resultMatrix[0][0], sizeof(int32_t) * DISPLAY_MATRIX_ROWS * DISPLAY_MATRIX_COLS,
                &trackDisplayMatrix[0][0], sizeof(int32_t) * DISPLAY_MATRIX_ROWS * DISPLAY_MATRIX_COLS) != EOK) {
                MEDIA_LOG_E("Failed to copy track display matrix");
                return Status::ERROR_INVALID_DATA;
            }
        }
        SetTrackDisplayMatrix(lastTrack_, resultMatrix, width, height);
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

Status ParseElstEntry(const uint8_t* elstInfo, uint32_t index, uint8_t version,
    int64_t& segmentDuration, int64_t& mediaTime)
{
    constexpr uint32_t elstHeaderSize = 8;
    constexpr uint32_t entrySizeV0 = 12;
    constexpr uint32_t entrySizeV1 = 20;
    uint32_t entrySize = (version == 1) ? entrySizeV1 : entrySizeV0;
    uint32_t entryOffset = elstHeaderSize + index * entrySize;
    if (version == 1) {
        segmentDuration = static_cast<int64_t>(GetU64Value(&elstInfo[entryOffset]));
        mediaTime = static_cast<int64_t>(GetU64Value(&elstInfo[entryOffset + sizeof(uint64_t)]));
    } else {
        int32_t tempDuration = static_cast<int32_t>(GetU32Value(&elstInfo[entryOffset]));
        int32_t tempMediaTime = static_cast<int32_t>(GetU32Value(&elstInfo[entryOffset + sizeof(uint32_t)]));
        segmentDuration = tempDuration;
        mediaTime = tempMediaTime;
    }
    MEDIA_LOG_D("Parsed elst entry " PUBLIC_LOG_U32 ": segmentDuration=" PUBLIC_LOG_U64 ", mediaTime=" PUBLIC_LOG_D64,
        index, segmentDuration, mediaTime);
    FALSE_RETURN_V_MSG_E(mediaTime >= 0 || mediaTime == -1, Status::ERROR_INVALID_DATA,
        "Invalid edit list media time: " PUBLIC_LOG_D64, mediaTime);
    
    return Status::OK;
}

Status MPEG4AtomParser::ParseElst(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    ctx->offset += currentAtom.size;
    constexpr uint32_t maxEntryNum = 2; // elst 支持最多2个条目

    auto elstInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, elstInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read elst info failed");
    uint8_t version = elstInfo[0];
    uint32_t entryCount = GetU32Value(&elstInfo[4]);
    FALSE_RETURN_V_MSG_E(entryCount > 0 && entryCount <= maxEntryNum, Status::ERROR_INVALID_DATA,
        "Invalid entry count: " PUBLIC_LOG_U32, entryCount);

    uint32_t elstStartIndex = 0;
    for (uint32_t i = 0; i < entryCount; ++i) {
        int64_t segmentDuration = 0;
        int64_t mediaTime = 0;

        ret = ParseElstEntry(elstInfo.get(), i, version, segmentDuration, mediaTime);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse elst entry failed");
        lastTrack_->totalElstDuration += segmentDuration;
        if (entryCount == maxEntryNum) {
            if (i == 0 && mediaTime == -1) {
                lastTrack_->elstInitEmptyEdit = segmentDuration;
                elstStartIndex = 1;
            } else if (i == elstStartIndex && mediaTime >= 0) {
                lastTrack_->elstShiftStartTime = mediaTime;
            } else {
                MEDIA_LOG_E("Invalid elst entry");
                return Status::ERROR_INVALID_DATA;
            }
        } else if (entryCount == 1) {
            if (segmentDuration >= 0 && mediaTime != -1) {
                lastTrack_->elstInitEmptyEdit = 0;
                lastTrack_->elstShiftStartTime = mediaTime;
            } else {
                MEDIA_LOG_E("Invalid elst entry");
                return Status::ERROR_INVALID_DATA;
            }
        }
    }

    int32_t fileTimeScale = 0;
    int64_t startTime = 0;
    if (mediaInfo_.general.Get<Tag::MEDIA_TIME_SCALE>(fileTimeScale) && fileTimeScale > 0) {
        FALSE_RETURN_V_MSG_E(
            ConvertTimeScale(lastTrack_->elstInitEmptyEdit, S_TO_US, fileTimeScale, &startTime) == Status::OK,
            Status::ERROR_INVALID_PARAMETER, "Mul overflow");
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_START_TIME>(startTime);
    return ret;
}

Status MPEG4AtomParser::ParseTref(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_TWO, Status::ERROR_INVALID_DATA, "Invalid tref depth");
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse tref container failed");
    return ret;
}

Status MPEG4AtomParser::ParseCdsc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    uint32_t idCount = currentAtom.dataSize / 4;
    FALSE_RETURN_V_MSG_E(idCount > 0, Status::ERROR_INVALID_DATA, "Invalid id count in cdsc atom");

    auto idData = std::make_unique<uint8_t[]>(idCount * sizeof(uint32_t));
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, idData.get(), idCount * sizeof(uint32_t));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read cdsc atom id data failed");
    std::vector<uint32_t> referencedTrackIds;
    std::string referencedIdsStr;
    for (uint32_t i = 0; i < idCount; ++i) {
        uint32_t trackId = GetU32Value(&idData[i * sizeof(uint32_t)]);
        if (trackId > 0) {
            referencedTrackIds.emplace_back(trackId - 1);
            if (!referencedIdsStr.empty()) {
                referencedIdsStr += ",";
            }
            referencedIdsStr += std::to_string(trackId - 1);
        } else {
            MEDIA_LOG_W("Invalid track ID: " PUBLIC_LOG_U32, trackId);
        }
    }

    if (!referencedTrackIds.empty()) {
        int32_t referencedTrackId = referencedTrackIds[0];
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::TIMED_METADATA_SRC_TRACK>(referencedTrackId);
        MEDIA_LOG_D("Referenced track IDs: " PUBLIC_LOG_S, referencedIdsStr.c_str());
    }

    ctx->offset += currentAtom.size;
    return ret;
}

Status MPEG4AtomParser::ParseMdia(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_TWO, Status::ERROR_INVALID_DATA, "Invalid mdia depth");
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse mdia container failed");
    return ret;
}

Status MPEG4AtomParser::ParseMdhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "current track is null");
    constexpr uint64_t minMdhdSize = 24; // 最小mdhd atom大小
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= minMdhdSize, Status::ERROR_INVALID_DATA, "Invalid mdhd atom size");
    ctx->offset += currentAtom.size;

    auto headerInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read mdhd info failed");

    uint8_t version = headerInfo[0];
    FALSE_RETURN_V_MSG_E(version == 0 || version == 1, Status::ERROR_INVALID_DATA, "Invalid mdhd version");
    // skip some unused attributes
    uint32_t timeScaleOffset = (version == 1) ? OFFSET_V1 : OFFSET_V0;
    uint32_t timeScale = GetU32Value(&headerInfo[timeScaleOffset]);
    if (timeScale <= 0) {
        MEDIA_LOG_E("Invalid timescale");
        timeScale = 1;  // set default timescale to 1
    }
    int64_t duration = 0;
    uint32_t durationOffset = (version == 1) ? DURATION_OFFSET_V1 : DURATION_OFFSET_V0;
    if (version == 1) {
        duration = GetU64Value(&headerInfo[durationOffset]);
    } else if (version == 0) {
        duration = GetU32Value(&headerInfo[durationOffset]);
    }
    lastTrack_->duration = duration;
    if (!IsValidTrackIndex(mediaInfo_, lastTrack_->trackIndex)) {
        MEDIA_LOG_E("Track index out of bounds: " PUBLIC_LOG_U32, lastTrack_->trackIndex);
        return Status::ERROR_INVALID_DATA;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_TIME_SCALE>(static_cast<int32_t>(timeScale));
    //duration  4 bytes or 8 bytes
    uint16_t languageValue;
    constexpr int32_t languageOffsetVersionOne = 32;
    constexpr int32_t languageOffsetVersionZero = 20;
    if (version == 1) {
        languageValue = GetU16Value(&headerInfo[languageOffsetVersionOne]);
    } else {
        languageValue = GetU16Value(&headerInfo[languageOffsetVersionZero]);
    }

    std::string langString = ConvertLanguageCode(languageValue);
    MEDIA_LOG_D("Language code: " PUBLIC_LOG_S, langString.c_str());
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_LANGUAGE>(langString);
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
    if (path.size() >= DIRECT_META_PATH_SIZE && path[0] == FourccType("moov") && path[1] == FourccType("meta")) {
        return (depth == MPEG4_DEPTH_TWO || (depth == MPEG4_DEPTH_THREE &&
            path.size() >= metaPathSize && path[pathTwo] == FourccType("hdlr")));
    }
    
    // 情况2: moov/udta/meta 结构
    if (path.size() >= metaPathSize && path[0] == FourccType("moov") &&
        path[1] == FourccType("udta") && path[pathTwo] == FourccType("meta")) {
        return (depth == MPEG4_DEPTH_THREE || (depth == MPEG4_DEPTH_FOUR &&
            path.size() >= hdlrPathSize && path[pathThree] == FourccType("hdlr")));
    }
    
    return false;
}

Status MPEG4AtomParser::ParseHdlr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_HDLR_DATA_SIZE, Status::ERROR_INVALID_DATA, "Invalid size");
    // Skip version and flags (4 bytes), then read componentType and handlerType
    ctx->offset += currentAtom.size;
    uint8_t typeInfo[12];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, typeInfo, sizeof(typeInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read hdlr type info failed");
    uint32_t componentType = GetU32Value(&typeInfo[4]);
    uint32_t handlerType = GetU32Value(&typeInfo[8]);
    // If this is a metadata handler outside a valid track, mark it and return
    if (UnderMetaPath(ctx->path, depth)) {
        ctx->foundHdlrMdta = handlerType == FourccType("mdta") ? true : false;
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "not in a valid track");
    switch (handlerType) {
        case FourccType("vide"):
            lastTrack_->codecParms.trackType = VIDEO_TYPE;
            break;
        case FourccType("soun"):
            lastTrack_->codecParms.trackType = AUDIO_TYPE;
            break;
        case FourccType("subp"):
        case FourccType("clcp"):
        case FourccType("text"):
            lastTrack_->codecParms.trackType = SUBTITLE_TYPE;
            break;
        case FourccType("meta"):
            ret = ProcessMetaHandler(currentAtom, ctx);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Process meta handler failed");
            break;
        default:
            if (componentType == FourccType("mhlr") && handlerType == FourccType("MPEG")) {
                MEDIA_LOG_E("Unsupported MPEG handler type, it's MPEGPS in MOV ");
                return Status::ERROR_UNSUPPORTED_FORMAT;
            } else if (ctx->path.size() > DIRECT_META_PATH_SIZE &&
                       ctx->path[ctx->path.size() - DIRECT_META_PATH_SIZE] != FourccType("mdia")) {
                return Status::OK;
            }
            lastTrack_->codecParms.trackType = INVALID_TYPE;
            break;
    }
    lastTrack_->sampleHelper->trackType_ = lastTrack_->codecParms.trackType;
    MediaType mediaType = static_cast<MediaType>(lastTrack_->codecParms.trackType);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_TYPE>(mediaType);
    return ret;
}

Status MPEG4AtomParser::ProcessMetaHandler(MPEG4Atom currentAtom, ParseContext* ctx)
{
    char titleBuffer[MAX_TITLE_SIZE + 1] = {'\0'};
    uint64_t titleSize = std::min(static_cast<uint64_t>(MAX_TITLE_SIZE), currentAtom.dataSize - MAX_TITLE_SIZE);
    if (titleSize <= 0) {
        return Status::OK;
    }
    
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + MAX_TITLE_SIZE,
        reinterpret_cast<uint8_t*>(titleBuffer), titleSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read meta handler title failed");
    
    int32_t offset = (!isIsom_ && static_cast<uint8_t>(titleBuffer[0]) == titleSize - 1) ? 1 : 0;
    constexpr uint32_t timedMetaDataLength = 14;

    if (titleSize >= static_cast<uint64_t>(offset + timedMetaDataLength) &&
        strncmp(titleBuffer + offset, "timed_metadata", timedMetaDataLength) == 0) {
        lastTrack_->codecParms.trackType = TIMEDMETA_TYPE;
        lastTrack_->sampleHelper->trackType_ = TIMEDMETA_TYPE;
    }
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

Status MPEG4AtomParser::ParseStsd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE, Status::ERROR_INVALID_DATA, "Invalid atom size");

    uint8_t stsdData[8];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, stsdData, sizeof(stsdData));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read stsd entry count failed");
    uint32_t entryCount = GetU32Value(&stsdData[4]);
    if (entryCount <= 0 || entryCount > 0x0400) {
        MEDIA_LOG_E("Invalid STSD entries");
        return Status::ERROR_INVALID_DATA;
    }
    int64_t endOffset = ctx->dataOffset + currentAtom.dataSize;
    ctx->offset = ctx->dataOffset + sizeof(stsdData); // 跳过stsd头部信息
    // 读取sample description entries
    for (uint32_t i = 0; i < entryCount; ++i) {
        int64_t atomOffset = ctx->offset;
        uint8_t sampleEntryHeader[8];
        ret = dataReader_->ReadUintData(atomOffset, sampleEntryHeader, sizeof(sampleEntryHeader));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sample entry header failed");
        MPEG4Atom atom{
            .size = GetU32Value(&sampleEntryHeader[0]),
            .type = static_cast<int32_t>(GetU32Value(&sampleEntryHeader[4])),
            .dataSize = GetU32Value(&sampleEntryHeader[0]) - sizeof(sampleEntryHeader)
        };
        ctx->dataOffset = atomOffset + sizeof(sampleEntryHeader);
        MEDIA_LOG_D("Sample Entry type: " PUBLIC_LOG_S, FourccToString(atom.type).c_str());
        if (lastTrack_->codecParms.trackType == VIDEO_TYPE) {
            ret = ParseVideoSampleEntry(atom, depth + 1, ctx);
        } else if (lastTrack_->codecParms.trackType == AUDIO_TYPE) {
            ret = ParseAudioSampleEntry(atom, depth + 1, ctx);
        } else if (lastTrack_->codecParms.trackType == SUBTITLE_TYPE) {
            ret = ParseSubtitleSampleEntry(atom, depth + 1, ctx);
        } else if (lastTrack_->codecParms.trackType == TIMEDMETA_TYPE) {
            ret = ParseMebx(atom, depth + 1, ctx);
        } else {
            MEDIA_LOG_W("Unsupported track type: " PUBLIC_LOG_D32, lastTrack_->codecParms.trackType);
            std::string mimeType = MimeType::INVALID_TYPE;
            lastTrack_->sampleHelper->mimeType_ = mimeType;
            mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimeType);
            ctx->offset += atom.size;
            continue;
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse stsd container failed");
    }
    FALSE_RETURN_V_MSG_E(ctx->offset == endOffset, Status::ERROR_INVALID_DATA, "Invalid stsd size");
    if (lastTrack_->codecParms.trackType == AUDIO_TYPE) {
        ret = SetAudioAttribute();
    }
    return ret;
}

// 该解析函数专门用于处理视频SampleEntry atom，如avc1 hvc1 vvc1 enca
Status MPEG4AtomParser::ParseVideoSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_INVALID_DATA, "Track is nullptr");
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
        lastTrack_->sampleHelper->mimeType_ = mimeType;
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimeType);
    }

    if (HDR_VIVID_TAG_OFFSET + HDR_VIVID_TAG_LENGTH <= sizeof(sampleEntryInfo)) {
        char hdrTag[HDR_VIVID_TAG_LENGTH];
        for (int64_t i = 0; i < HDR_VIVID_TAG_LENGTH; ++i) {
            hdrTag[i] = sampleEntryInfo[HDR_VIVID_TAG_OFFSET + i];
        }
        if (!strncmp(hdrTag, "CUVA HDR Video", strlen("CUVA HDR Video"))) {
            mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::VIDEO_IS_HDR_VIVID>(true);
        }
    } else {
        MEDIA_LOG_E("Invalid HDR tag offset or length");
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
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    if (currentAtom.type == FourccType("wvtt")) {
        std::string mimeType = GetMimeType(currentAtom.type);
        lastTrack_->sampleHelper->mimeType_ = mimeType;
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimeType);
    }
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseMebx(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_TIMED_METADATA_SAMPLE_ENTRY_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid mebx atom size");
    constexpr uint64_t skipSize = 8;
    // 设置MIME类型
    std::string mimeType = GetMimeType(currentAtom.type);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimeType);

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
    ctx->offset += currentAtom.size;
    MEDIA_LOG_D("Mebx parsing completed. Key: " PUBLIC_LOG_S, timedMetadataKey.c_str());
    return Status::OK;
}

// 该解析函数专门用于处理CodecConfig atom，如avcC hvcC vvcC
Status MPEG4AtomParser::ParseCodecConfig(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize <= MAX_EXTRA_DATA_SIZE, Status::ERROR_INVALID_DATA,
        "CodecConfig size too large");
    ctx->offset += currentAtom.size;
    // 检查是否已经有 codec config 数据
    if (lastTrack_->codecParms.data != nullptr) {
        MEDIA_LOG_W("CodecConfig already exists, replacing with new data. Old size: " PUBLIC_LOG_D64 ", New size: "
            PUBLIC_LOG_D64, lastTrack_->codecParms.extradataSize, currentAtom.dataSize);
        lastTrack_->codecParms.data.reset();
        lastTrack_->codecParms.extradataSize = 0;
    }

    auto configData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(configData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate CodecConfig extradata");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, configData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_NOT_ENOUGH_DATA, "Read CodecConfig data failed");

    lastTrack_->codecParms.data = std::move(configData);
    lastTrack_->codecParms.extradataSize = currentAtom.dataSize;
    MEDIA_LOG_D("CodecConfig extradata size: " PUBLIC_LOG_D64, lastTrack_->codecParms.extradataSize);
    return ret;
}

// 该解析函数专门用于处理音频SampleEntry atom，如mp4a enca
Status MPEG4AtomParser::ParseAudioSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");

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

AudioSampleFormat GetLpcmSampleFormaForFloat(uint32_t bigEndian, uint16_t sampleSize)
{
    switch (sampleSize) {
        case 0x20:
            return bigEndian ? AudioSampleFormat::SAMPLE_F32BE : AudioSampleFormat::SAMPLE_F32LE;
        case 0x40:
            return bigEndian ? AudioSampleFormat::SAMPLE_F64BE : AudioSampleFormat::INVALID_WIDTH;
        default:
            return AudioSampleFormat::INVALID_WIDTH;
    }
}

AudioSampleFormat GetLpcmSampleFormaForSigned(uint32_t bigEndian, uint16_t sampleSize)
{
    switch (sampleSize) {
        case 0x01:
            return AudioSampleFormat::SAMPLE_S8;
        case 0x02:
            return bigEndian ? AudioSampleFormat::SAMPLE_S16BE : AudioSampleFormat::SAMPLE_S16LE;
        case 0x03:
            return bigEndian ? AudioSampleFormat::SAMPLE_S24BE : AudioSampleFormat::SAMPLE_S24LE;
        case 0x04:
            return bigEndian ? AudioSampleFormat::SAMPLE_S32BE : AudioSampleFormat::SAMPLE_S32LE;
        case 0x08:
            return bigEndian ? AudioSampleFormat::SAMPLE_F64BE : AudioSampleFormat::INVALID_WIDTH;
        default:
            return AudioSampleFormat::INVALID_WIDTH;
    }
}

AudioSampleFormat GetLpcmSampleFormaForOther(uint16_t sampleSize)
{
    switch (sampleSize) {
        case 0x01:
            return AudioSampleFormat::SAMPLE_U8;
        default:
            return AudioSampleFormat::INVALID_WIDTH;
    }
}

AudioSampleFormat GetLpcmSampleFormat(uint16_t sampleSize, uint32_t flags)
{
    constexpr uint16_t maxSampleSize = 64;
    constexpr uint16_t extraSampleSizeAdd = 7;
    uint32_t floatLE = flags & 0x01;
    uint32_t bigEndian = flags & 0x02;
    int32_t extraflag = flags & 0x04 ? -1 : 0;
    if (sampleSize <= 0 || sampleSize > maxSampleSize) {
        return AudioSampleFormat::INVALID_WIDTH;
    }
    if (floatLE) {
        return GetLpcmSampleFormaForFloat(bigEndian, sampleSize);
    } else {
        sampleSize += extraSampleSizeAdd;
        sampleSize >>= 0x03;
        if (extraflag & (1 << (sampleSize - 1))) {
            return GetLpcmSampleFormaForSigned(bigEndian, sampleSize);
        } else {
            return GetLpcmSampleFormaForOther(sampleSize);
        }
    }
}

Status MPEG4AtomParser::ParseAudioAttributes(MPEG4Atom currentAtom, ParseContext* ctx)
{
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
    //skip unneed distribute
    uint32_t sampleRate = GetU32Value(&sampleEntryInfo[0x18]) / CONVERT_SCALE;
    ctx->offset = ctx->dataOffset + sizeof(sampleEntryInfo);
    // 处理不同版本的扩展数据
    constexpr int32_t versionOneSikpSize = 16;
    constexpr int32_t versionTwoSikpSize = 36;
    constexpr uint16_t sampleSizeOffset = 20;
    constexpr uint16_t versionTwo = 2;
    uint32_t samplePerFrame = 0;
    if (!isIsom_ || (ctx->compatibleBrands.count(FourccType("qt  "))) > 0 || version > 0) {
        size_t entryInfoSize = (version == 1) ? versionOneSikpSize : versionTwoSikpSize;
        auto entryInfo = std::make_unique<uint8_t[]>(entryInfoSize);
        ret = dataReader_->ReadUintData(ctx->offset, entryInfo.get(), entryInfoSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read entry info failed");
        if (version == 1) {
            samplePerFrame = GetU32Value(&entryInfo[0]);
            ctx->offset += versionOneSikpSize;
        } else if (version == versionTwo) {
            //skip struct 4bytes
            sampleRate = static_cast<uint32_t>(GetDoubleValue(&entryInfo[0x04]));
            channelCount = static_cast<uint16_t>(GetU32Value(&entryInfo[0x0C]));
            // skip 4byets
            sampleSize = static_cast<uint16_t>(GetU32Value(&entryInfo[sampleSizeOffset]));
            uint32_t flags = GetU32Value(&entryInfo[0x18]);
            samplePerFrame = GetU32Value(&entryInfo[0x20]);
            if (currentAtom.type == FourccType("lpcm")) {
                AudioSampleFormat sampleFormat = GetLpcmSampleFormat(sampleSize, flags);
                mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
            }
            ctx->offset += versionTwoSikpSize;
        }
    }
    SetAudioMimeTypeAndSampleSize(currentAtom, sampleSize);

    int32_t finalSampleSize = static_cast<int32_t>(sampleSize);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(channelCount));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(sampleRate));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(finalSampleSize);
    if (samplePerFrame) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(
            static_cast<int32_t>(samplePerFrame));
    }

    if (isIsom_ && currentAtom.type == FourccType("alac")) {
        ret = ParseAlacEntry(*ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse ALAC entry failed");
    }

    return ret;
}

void MPEG4AtomParser::SetAmrAudioAttribute(std::string mime)
{
    if (mime == MimeType::AUDIO_AMR_NB) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    } else {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
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

Status MPEG4AtomParser::SetVorbisAudioAttribute()
{
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
    uint8_t* vorbisData = lastTrack_->codecParms.data.get();
    uint32_t currentOffset = 4;
    if (vorbisData[currentOffset] != 0x76 || vorbisData[currentOffset + 0x01] != 0x6F ||
        vorbisData[currentOffset + 0x02] != 0x72 || vorbisData[currentOffset + 0x03] != 0x62 ||
        vorbisData[currentOffset + 0x04] != 0x69 || vorbisData[currentOffset + 0x05] != 0x73) {
        return Status::ERROR_INVALID_DATA;
    }
    currentOffset += 0x0A;
    int32_t channels = vorbisData[currentOffset];
    AudioChannelLayout channelLayout = FLAC_CHANNEL_LAYOUT_TABLE.find(channels) -> second;
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
    return Status::OK;
}

Status MPEG4AtomParser::SetAudioAttribute()
{
    int32_t sampleRate = 0;
    int32_t timescale = 0;
    mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timescale);
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
    } else if (mime == MimeType::AUDIO_VORBIS) {
        ret = SetVorbisAudioAttribute();
    } else if (mime == "audio/dts" || mime == MimeType::AUDIO_AAC) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
    }
    return ret;
}

void MPEG4AtomParser::SetAudioMimeTypeAndSampleSize(const MPEG4Atom& currentAtom, uint16_t& sampleSize)
{
    if (currentAtom.type != FourccType("enca")) {
        std::string mimeType = GetMimeType(currentAtom.type);
        lastTrack_->sampleHelper->mimeType_ = mimeType;
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimeType);
        if (mimeType == MimeType::AUDIO_G711MU || mimeType == MimeType::AUDIO_G711A) {
            sampleSize = G711MULAW_SAMPLE_SIZE; // G.711 mu-law uses 8 bits per sample
            mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
        } else if (mimeType == MimeType::AUDIO_RAW) {
            sampleSize = GetRawPCMBitsPerSample(currentAtom, sampleSize);
            SetRawPCMSampleFormat(currentAtom, sampleSize);
        }
    }
}

int32_t MPEG4AtomParser::GetRawPCMBitsPerSample(const MPEG4Atom& currentAtom, uint16_t& sampleSize)
{
    constexpr uint16_t bitPCM8 = 8;
    constexpr uint16_t bitPCM16 = 16;
    constexpr uint16_t bitPCM24 = 24;
    constexpr uint16_t bitPCM32 = 32;
    constexpr uint16_t bitPCM64 = 64;

    switch (currentAtom.type) {
        case FourccType("raw "):
        case FourccType("NONE"):
            return bitPCM8;
        case FourccType("twos"):
        case FourccType("lpcm"):
        case FourccType("ipcm"):
            return bitPCM16;
        case FourccType("in24"):
            return bitPCM24;
        case FourccType("in32"):
        case FourccType("fl32"):
            return bitPCM32;
        case FourccType("fl64"):
            return bitPCM64;
        case FourccType("sowt"):
            if (sampleSize == 0x08) {
                return bitPCM8;
            } else if (sampleSize == 0x10) {
                return bitPCM16;
            }
            return 0;
        default:
            return 0;
    }
}

void MPEG4AtomParser::SetRawPCMSampleFormat(const MPEG4Atom& currentAtom, uint16_t& sampleSize)
{
    AudioSampleFormat sampleFormat = AudioSampleFormat::INVALID_WIDTH;
    if (mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat)) {
        return;
    }
    switch (currentAtom.type) {
        case FourccType("raw "):
        case FourccType("NONE"):
            sampleFormat = AudioSampleFormat::SAMPLE_U8;
            break;
        case FourccType("twos"):
        case FourccType("lpcm"):
        case FourccType("ipcm"):
            sampleFormat = AudioSampleFormat::SAMPLE_S16BE;
            break;
        case FourccType("in24"):
            sampleFormat = AudioSampleFormat::SAMPLE_S24BE;
            break;
        case FourccType("in32"):
            sampleFormat = AudioSampleFormat::SAMPLE_S32BE;
            break;
        case FourccType("fl32"):
            sampleFormat = AudioSampleFormat::SAMPLE_F32BE;
            break;
        case FourccType("fl64"):
            sampleFormat = AudioSampleFormat::SAMPLE_F64BE;
            break;
        case FourccType("sowt"):
            if (sampleSize == 0x08) {
                sampleFormat = AudioSampleFormat::SAMPLE_U8;
            } else if (sampleSize == 0x10) {
                sampleFormat = AudioSampleFormat::SAMPLE_S16LE;
            }
            break;
        default:
            sampleFormat = AudioSampleFormat::INVALID_WIDTH;
            break;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
}

Status MPEG4AtomParser::ParseAlacEntry(ParseContext& ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    constexpr uint32_t alacHeaderSize = 12; // ALAC header size
    constexpr uint32_t typeOffset = 4;      // ALAC type offset in the header
    constexpr uint32_t flagOffset = 8;      // ALAC flag offset in the header
    int64_t dataOffset = ctx.offset;
    auto alacData = std::make_unique<uint8_t[]>(alacHeaderSize);
    Status ret = dataReader_->ReadUintData(dataOffset, alacData.get(), alacHeaderSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read ALAC entry data failed");

    // 解析ALAC头部信息
    uint32_t size = GetU32Value(&alacData[0]);
    uint32_t type = GetU32Value(&alacData[typeOffset]);
    uint32_t flag = GetU32Value(&alacData[flagOffset]);
    if (size != ALAC_SPECIFIC_SIZE || type != FourccType("alac") || flag != 0) {
        MEDIA_LOG_E("Invalid ALAC Entry");
        return Status::ERROR_INVALID_DATA;
    }
    dataOffset += alacHeaderSize;
    uint8_t buffer[size - alacHeaderSize];
    ret = dataReader_->ReadUintData(dataOffset, buffer, sizeof(buffer));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read buffer data failed");
    dataOffset += sizeof(buffer);
    // 解析ALAC的 sample size 音频通道数和采样率
    uint8_t sampleSize = buffer[5];
    uint8_t channelCount = buffer[9];
    uint32_t sampleRate = GetU32Value(&buffer[20]);
    if (channelCount > ALAC_CHANNEL_LAYOUT_TABLE.size()) {
        MEDIA_LOG_E("Invalid ALAC channel count");
        return Status::ERROR_INVALID_DATA;
    }
    AudioChannelLayout channelLayout = ALAC_CHANNEL_LAYOUT_TABLE.find(channelCount)->second;
    switch (sampleSize) {
        case 0x10:
            mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16P);
            break;
        case 0x14:
        case 0x18:
        case 0x20:
            mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32P);
            break;
        default:
            MEDIA_LOG_E("Invalid sample size");
            return Status::ERROR_INVALID_DATA;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(static_cast<int32_t>(sampleSize));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(channelCount));
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(sampleRate));
    ctx.offset = dataOffset;
    return Status::OK;
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
        if (offset >= dataSize || count >= maxCount) {
            return false;
        }
        temp = data[offset++];
        size = (size << 0x07) | (temp & 0x7F);
        count++;
    } while (temp & 0x80);

    return true;
}

Status MPEG4AtomParser::ParseEsds(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    ctx->offset += currentAtom.size;
    
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
    FALSE_RETURN_V_MSG_E(ParseDecoderConfigDescriptor(esdsData.get(), currentAtom.dataSize, offset),
        Status::ERROR_INVALID_DATA, "Failed to parse decoder config descriptor");
    
    return Status::OK;
}

std::string GetMimeTypeFromObjectType(uint8_t objectTypeId)
{
    static const std::map<uint8_t, std::string> objectTypeToMimeMap = {
        // 视频编解码器
        {0x20, MimeType::VIDEO_MPEG4},    // MPEG-4 Visual
        {0x21, MimeType::VIDEO_AVC},      // H.264/AVC
        {0x23, MimeType::VIDEO_HEVC},     // H.265/HEVC
        {0x33, MimeType::VIDEO_VVC},      // H.266/VVC (OHOS扩展)
        // MPEG-2 Video系列
        {0x60, MimeType::VIDEO_MPEG2},    // MPEG-2 Simple Profile
        {0x61, MimeType::VIDEO_MPEG2},    // MPEG-2 Main Profile
        {0x62, MimeType::VIDEO_MPEG2},    // MPEG-2 SNR Profile
        {0x63, MimeType::VIDEO_MPEG2},    // MPEG-2 Spatial Profile
        {0x64, MimeType::VIDEO_MPEG2},    // MPEG-2 High Profile
        {0x65, MimeType::VIDEO_MPEG2},    // MPEG-2 422 Profile
        // 音频编解码器
        {0x40, MimeType::AUDIO_AAC},      // MPEG-4 AAC
        {0x66, MimeType::AUDIO_AAC},      // MPEG-2 AAC Main
        {0x67, MimeType::AUDIO_AAC},      // MPEG-2 AAC Low Complexity
        {0x68, MimeType::AUDIO_AAC},      // MPEG-2 AAC SSR
        {0x69, MimeType::AUDIO_MPEG},     // MPEG-2 Layer 3 (MP3) / MPEG-1 Layer 2 (MP2)
        {0x6B, MimeType::AUDIO_MPEG},     // MPEG-1 Layer 3 (MP3)
        {0xC1, MimeType::AUDIO_FLAC},     // FLAC
        {0xDD, MimeType::AUDIO_VORBIS},   // Vorbis
        {0xAD, MimeType::AUDIO_OPUS},     // Opus
        {0xA9, "audio/dts"}               // DTS
    };
    auto it = objectTypeToMimeMap.find(objectTypeId);
    return (it != objectTypeToMimeMap.end()) ? it->second : "";
}

bool MPEG4AtomParser::ParseDecoderConfigDescriptor(const uint8_t* data, uint64_t dataSize, uint64_t& offset)
{
    FALSE_RETURN_V_MSG_E(offset + ESDS_READ_SIZE <= dataSize, false, "Parse decoderConfig descriptor failed");

    uint8_t objectTypeId = data[offset++];         // Object Type ID
    // 跳过 streamType(1字节) buffer size (3字节) 和 max bitrate (4字节)
    offset += sizeof(uint64_t);

    std::string mimetype = GetMimeTypeFromObjectType(objectTypeId);
    if (!mimetype.empty()) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MIME_TYPE>(mimetype);
        lastTrack_->sampleHelper->mimeType_ = mimetype;
    }
    uint32_t avgBitrate = GetU32Value(&data[offset]);
    offset += sizeof(uint32_t);

    if (lastTrack_ && avgBitrate > 0) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(avgBitrate));
    }
    // 读取解码器特定信息描述符
    uint8_t tag;
    uint32_t size;
    FALSE_RETURN_V_MSG_E(ReadDescriptor(data, dataSize, offset, tag, size), false, "No decoder specific info found");

    if (tag == MPEG4_DEC_SPECIFIC_DESCR_TAG) {
        // 特殊情况：MPEG-1/2 Audio 不需要 decoder specific info
        const uint32_t maxSize = 1 << 30;
        if (objectTypeId == 0x69 || objectTypeId == 0x6B) {
            MEDIA_LOG_D("MPEG-1/2 Audio detected, no decoder specific info needed");
            return true;
        }

        FALSE_RETURN_V_MSG_E(size > 0 && size <= maxSize, false, "Wrong descriptor size");
        FALSE_RETURN_V_MSG_E(offset + size <= dataSize, false, "Decoder specific info extends beyond atom boundary");
        
        // 提取 decoder specific info
        const uint8_t* specificInfo = &data[offset];
        if (specificInfo && size > 0) {
            if (lastTrack_->codecParms.data) {
                lastTrack_->codecParms.data.reset();
            }
            lastTrack_->codecParms.data = std::make_unique<uint8_t[]>(size);
            FALSE_RETURN_V_MSG_E(lastTrack_->codecParms.data, false, "Failed to allocate codec config");
            
            auto ret = memcpy_s(lastTrack_->codecParms.data.get(), size, specificInfo, size);
            FALSE_RETURN_V_MSG_E(ret == EOK, false, "Failed to copy codec config data");
            lastTrack_->codecParms.extradataSize = size;
        }
        if (mimetype == MimeType::AUDIO_AAC) {
            return ParseAACSpecificConfig(specificInfo, size);
        }
    }
    
    return true;
}

void GetAACConfigExtInfo(const std::shared_ptr<DemuxerBitReader> bitReader, uint8_t& audioObjectType)
{
    bitReader->SkipBits(0x04);
    audioObjectType = bitReader->ReadBits(0x05);
    if (audioObjectType == 0x1F) {
        audioObjectType = 0x20 + bitReader->ReadBits(0x06);
    }
}

bool DecodeChannelMap(uint8_t layoutMap[][3], uint8_t channelType, const std::shared_ptr<DemuxerBitReader> bitReader,
    uint8_t num)
{
    uint8_t element;
    while (num--) {
        switch (channelType) {
            case 0x01:
            case 0x02:
            case 0x03: {
                element = bitReader->ReadBits(1);
                break;
            }
            case 0x04: {
                element = TYPE_LFE;
                break;
            }
            case 0x05: {
                bitReader->SkipBits(1);
                element = TYPE_CCE;
                break;
            }
            default:
                return false;
        }
        layoutMap[0][0] = element;
        layoutMap[0][1] = bitReader->ReadBits(0x04);
        layoutMap[0][0x02] = channelType;
        ++layoutMap;
    }
    return true;
}

uint8_t CountChannels(uint8_t (*layoutMap)[3], uint8_t tags)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < tags; ++i) {
        uint8_t element = layoutMap[i][0];
        uint8_t pos = layoutMap[i][0x02];
        sum += (1 + (element == TYPE_CPE)) * (pos != 0 && pos != 0x05);
    }
    return sum;
}

bool ParseGeneralAACConfig(const std::shared_ptr<DemuxerBitReader> bitReader, uint8_t audioObjectType,
    uint32_t& channels)
{
    uint8_t layoutMap[64][3];
    FALSE_RETURN_V(bitReader->HasBits(0x1A), false);
    uint32_t flag = bitReader->ReadBits(1);
    if (flag) {
        bitReader->SkipBits(0x19);
    } else {
        bitReader->SkipBits(0x0B);
    }
    FALSE_RETURN_V(bitReader->HasBits(0x23), false);
    uint8_t numFront = bitReader->ReadBits(0x04);
    uint8_t numSide = bitReader->ReadBits(0x04);
    uint8_t numBack = bitReader->ReadBits(0x04);
    uint8_t numLfe = bitReader->ReadBits(0x02);
    uint8_t numAssociateData = bitReader->ReadBits(0x03);
    uint8_t numCc = bitReader->ReadBits(0x04);
    if (bitReader->ReadBits(1)) {
        bitReader->ReadBits(0x04);
    }
    if (bitReader->ReadBits(1)) {
        bitReader->ReadBits(0x04);
    }
    if (bitReader->ReadBits(1)) {
        bitReader->ReadBits(0x03);
    }
    if (!(bitReader->HasBits(0x05 * (numFront + numSide + numBack + numCc) +
        0x04 * (numLfe + numAssociateData + numCc)))) {
        return false;
    }
    uint8_t tags = 0;
    FALSE_RETURN_V(DecodeChannelMap(layoutMap, 0x01, bitReader, numFront), false);
    tags += numFront;
    FALSE_RETURN_V(DecodeChannelMap(layoutMap + tags, 0x02, bitReader, numSide), false);
    tags += numSide;
    FALSE_RETURN_V(DecodeChannelMap(layoutMap + tags, 0x03, bitReader, numBack), false);
    tags += numBack;
    FALSE_RETURN_V(DecodeChannelMap(layoutMap + tags, 0x04, bitReader, numLfe), false);
    tags += numLfe;
    bitReader->SkipBits(0x04 * numAssociateData);
    FALSE_RETURN_V(DecodeChannelMap(layoutMap + tags, 0x05, bitReader, numCc), false);
    tags += numCc;

    channels = CountChannels(layoutMap, tags);
    return true;
}

bool CheckAACType(uint8_t audioObjectType)
{
    switch (audioObjectType) {
        case AAC_MAIN:
        case AAC_LC:
        case AAC_SSR:
        case AAC_LTP:
        case AAC_ER_LC:
        case AAC_ER_LD:
        case AAC_ER_ELD:
            return true;
        default:
            return false;
    }
}

bool GetAACLayoutConfig(uint8_t audioObjectType, const std::shared_ptr<DemuxerBitReader> bitReader, uint32_t size,
    uint32_t& channels, AudioChannelLayout &layout)
{
    switch (audioObjectType) {
        case AAC_MAIN:
        case AAC_LC:
        case AAC_SSR:
        case AAC_LTP:
        case AAC_ER_LC:
        case AAC_ER_LD: {
            FALSE_RETURN_V(ParseGeneralAACConfig(bitReader, audioObjectType, channels), false);
            break;
        }
        case AAC_ER_ELD: {
            return true;
        }
        default:
            return false;
    }
    return true;
}

bool MPEG4AtomParser::ParseAACConfig(const uint8_t* data, uint32_t size, uint32_t& sampleRate, uint32_t& channels,
    AudioChannelLayout &layout)
{
    auto bitReader = std::make_shared<DemuxerBitReader>(data, size);
    FALSE_RETURN_V_MSG_E(bitReader != nullptr, false, "Failed create bitReader");
    FALSE_RETURN_V(bitReader->HasBits(MIN_BOX_SIZE), false);
    uint8_t audioObjectType = bitReader->ReadBits(0x05);
    if (audioObjectType == 0x1F) {
        FALSE_RETURN_V(bitReader->HasBits(0x11), false);
        audioObjectType = 0x20 + bitReader->ReadBits(0x06);
    }
    uint8_t samplingFreqIndex = bitReader->ReadBits(0x04);
    if (samplingFreqIndex == 0x0F) {
        FALSE_RETURN_V(bitReader->HasBits(0x1F), false);
        sampleRate = bitReader->ReadBits(0x08) << 0x10 | bitReader->ReadBits(0x08) << 0x08 | bitReader->ReadBits(0x08);
    } else {
        if (samplingFreqIndex < sizeof(AAC_SAMPLE_RATE_TABLE) / sizeof(AAC_SAMPLE_RATE_TABLE[0])) {
            sampleRate = AAC_SAMPLE_RATE_TABLE[samplingFreqIndex];
        }
    }
    uint8_t channelConfig = bitReader->ReadBits(0x04);
    uint32_t frameLengthFlag = 0;
    static const uint8_t aacAudioChannels[14] = {
        0, 1, 2, 3, 4, 5, 6, 8, 0, 0, 0, 7, 8, 24
    };
    if (channelConfig < sizeof(aacAudioChannels) / sizeof(aacAudioChannels[0])) {
        channels = aacAudioChannels[channelConfig];
        layout = AAC_CHANNEL_LAYOUT_TABLE[channelConfig];
    } else {
        MEDIA_LOG_E("Invalid AAC channel configuration: " PUBLIC_LOG_U8, channelConfig);
        return false;
    }
    if (audioObjectType == AOT_SBR || (audioObjectType == AOT_PS &&
        (!(bitReader->ShowBits(0x03) & 0x03) && !(bitReader->ShowBits(0x09) & 0x0F)))) {
        FALSE_RETURN_V(bitReader->HasBits(0x09), false);
        GetAACConfigExtInfo(bitReader, audioObjectType);
    }
    frameLengthFlag = bitReader->ReadBits(1);
    if (channelConfig == 0) {
        FALSE_RETURN_V_MSG_E(GetAACLayoutConfig(audioObjectType, bitReader, size, channels, layout), false,
            "Get AAC layout config failed");
    }
    FALSE_RETURN_V_MSG_E(CheckAACType(audioObjectType), false, "Unsupported AAC type");
    int32_t samplePerFrame = frameLengthFlag ? 960 : 1024;
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
    constexpr uint32_t minConfigSize = 2;
    if (!data || size < minConfigSize) {
        MEDIA_LOG_E("Invalid AAC config data");
        return false;
    }
    uint32_t sampleRate = 44100;
    uint32_t channels = 2;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    if (ParseAACConfig(data, size, sampleRate, channels, layout)) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_COUNT>(static_cast<int32_t>(channels));
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_RATE>(static_cast<int32_t>(sampleRate));
        if (layout == AudioChannelLayout::UNKNOWN) {
            int32_t channelCount = 0;
            auto audioParser = std::make_shared<MPEG4AudioParser>();
            FALSE_RETURN_V_MSG_E(audioParser != nullptr, false, "Failed create audioParser");
            FALSE_RETURN_V_MSG_E(
                mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount),
                false, "Channel count missed");
            layout = audioParser->GetDefaultChannelLayout(channelCount);
        }
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
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
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    ctx->offset += currentAtom.size;

    uint8_t btrtInfo[8];
    // Skip version and flags (4 bytes)
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + 4, btrtInfo, sizeof(btrtInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read btrt info failed");
    uint32_t maxBitrate = GetU32Value(&btrtInfo[0]);
    uint32_t avgBitrate = GetU32Value(&btrtInfo[4]);
    MEDIA_LOG_D("Btrt: maxBitrate=" PUBLIC_LOG_U32 ", avgBitrate=" PUBLIC_LOG_U32, maxBitrate, avgBitrate);
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::MEDIA_BITRATE>(static_cast<int64_t>(avgBitrate));
    return ret;
}

Status MPEG4AtomParser::ParsePasp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE, Status::ERROR_INVALID_DATA, "Invalid pasp size");
    ctx->offset += currentAtom.size;

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

bool MPEG4AtomParser::ParseFLACExtradata(const uint8_t* data)
{
    const int32_t channelsOffset = 12;
    const int32_t bitsPerRawSampleOffset = 13;
    int32_t channels = ((data[channelsOffset] >> 1) & 0x07) + 1;
    int32_t bitsPerRawSample = (((data[channelsOffset] & 0x01) << 4) |
                                ((data[bitsPerRawSampleOffset] >> 4) & 0x0F)) + 1;
    const int32_t bitsPerRawSampleMin = 4;
    if (bitsPerRawSample < bitsPerRawSampleMin) {
        MEDIA_LOG_E("Invalid bitsPerRawSample: " PUBLIC_LOG_D32, bitsPerRawSample);
        return false;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(bitsPerRawSample);
    if (static_cast<size_t>(channels) <= FLAC_CHANNEL_LAYOUT_TABLE.size()) {
        AudioChannelLayout channelLayout = FLAC_CHANNEL_LAYOUT_TABLE.find(channels) -> second;
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
    } else {
        MEDIA_LOG_E("Invalid channels: " PUBLIC_LOG_D32, channels);
        return false;
    }
    const int32_t audio16BitSampleThreshold = 16;
    bool need32BitSampleFormat = bitsPerRawSample > audio16BitSampleThreshold;
    if (need32BitSampleFormat) {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S32LE);
    } else {
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    }
    return true;
}

Status MPEG4AtomParser::ParseDfla(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= 42 && currentAtom.dataSize <= (1UL << 30), Status::ERROR_INVALID_DATA,
        "dfla atom size out of range: " PUBLIC_LOG_U64, currentAtom.dataSize);

    ctx->offset += currentAtom.size;

    auto dflaData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(dflaData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate dfla buffer");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, dflaData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read dfla data failed");
    
    uint8_t version = dflaData[0];
    FALSE_RETURN_V_MSG_E(version == 0, Status::ERROR_INVALID_DATA, "Unsupported FLAC version");

    uint32_t offset = 4; // 跳过版本(1字节) 和 flags(3字节)
    uint8_t* blockHeader = &dflaData[offset]; // 解析 FLAC 元数据块头部
    uint8_t blockType = blockHeader[0] & 0x7F;
    uint32_t blockSize = (static_cast<uint32_t>(blockHeader[1]) << 0x10) |
                         (static_cast<uint32_t>(blockHeader[2]) << 0x08) |
                          static_cast<uint32_t>(blockHeader[3]);

    constexpr uint32_t streamInfoSize = 34;
    FALSE_RETURN_V_MSG_E(blockType == 0, Status::ERROR_INVALID_DATA,
        "StreamInfo must be first FLACMetadataBlock");
    FALSE_RETURN_V_MSG_E(blockSize == streamInfoSize, Status::ERROR_INVALID_DATA,
        "Invalid streamInfo size: " PUBLIC_LOG_U32 ", expected: " PUBLIC_LOG_U32, blockSize, streamInfoSize);
    
    offset += sizeof(streamInfoSize); // 跳过元数据块头部
    FALSE_RETURN_V_MSG_E(offset + blockSize <= currentAtom.dataSize, Status::ERROR_INVALID_DATA,
        "dfla atom truncated at streamInfo data");

    // 提取 StreamInfo 数据作为 extradata
    auto extradataSize = blockSize;
    auto extradata = std::make_unique<uint8_t[]>(extradataSize);
    FALSE_RETURN_V_MSG_E(extradata != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate FLAC extradata");
    if (memcpy_s(extradata.get(), extradataSize, &dflaData[offset], extradataSize) != EOK) {
        MEDIA_LOG_E("Failed to copy extradata");
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_E(ParseFLACExtradata(extradata.get()), Status::ERROR_INVALID_DATA,
        "Failed to parse FLAC extradata");
    lastTrack_->codecParms.data = std::move(extradata);
    lastTrack_->codecParms.extradataSize = extradataSize;
    return Status::OK;
}

bool MPEG4AtomParser::ParseOPUSExtradata(const uint8_t* data, uint64_t dataSize)
{
    constexpr uint64_t minExtradataSize = 19;
    FALSE_RETURN_V_MSG_E(dataSize >= minExtradataSize, false, "Invalid extradata size");
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
    int32_t channels = data[9];
    int32_t mapType = data[18];
    AudioChannelLayout channelLayout = AudioChannelLayout::UNKNOWN;
    constexpr int32_t maxChannels = 2;
    if (mapType == 0) {
        if (channels > maxChannels) {
            MEDIA_LOG_E("Invalid channels: " PUBLIC_LOG_D32, channels);
            return false;
        }
        channelLayout = (channels == 1) ? AudioChannelLayout::MONO :
                                          AudioChannelLayout::STEREO;
    } else if (mapType == 1) {
        channelLayout = FLAC_CHANNEL_LAYOUT_TABLE.find(channels)->second;
    }
    mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
    return true;
}

Status MPEG4AtomParser::ParseDops(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    // dops box至少11字节，最大1GB
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= OPUS_MIN_SIZE && currentAtom.dataSize <= MAX_SIZE,
        Status::ERROR_INVALID_DATA, "Invalid dops atom size");
    ctx->offset += currentAtom.size;
    auto dopsData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(dopsData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate dops buffer");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, dopsData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read dops data failed");
    // 检查 dops version
    uint8_t dopsVersion = dopsData[0];
    FALSE_RETURN_V_MSG_E(dopsVersion == 0, Status::ERROR_INVALID_DATA, "Unsupported OpusSpecificBox version");
    // 构造 OpusHead extradata
    constexpr uint64_t headLen = 8;
    constexpr uint64_t headVer = 1;
    constexpr std::string_view opusHeadName = "OpusHead";
    uint64_t opusHeadSize = currentAtom.dataSize + headLen;
    auto opusHead = std::make_unique<uint8_t[]>(opusHeadSize);
    FALSE_RETURN_V_MSG_E(opusHead != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate OpusHead buffer");
    if (memcpy_s(opusHead.get(), opusHeadName.size(), opusHeadName.data(), opusHeadName.size()) != EOK) {
        MEDIA_LOG_E("Failed to copy opus head name");
        return Status::ERROR_INVALID_DATA;
    }

    if (currentAtom.dataSize > 1) {
        size_t copySize = currentAtom.dataSize - headVer;
        size_t destSize = opusHeadSize - headLen - headVer;
        if (memcpy_s(opusHead.get() + headLen + headVer, destSize, dopsData.get() + headVer, copySize) != EOK) {
            MEDIA_LOG_E("Failed to copy extradata");
            return Status::ERROR_INVALID_DATA;
        }
    }
    FALSE_RETURN_V_MSG_E(ParseOPUSExtradata(opusHead.get(), opusHeadSize),
        Status::ERROR_INVALID_DATA, "Parse opus extradata failed");
    lastTrack_->codecParms.data = std::move(opusHead);
    lastTrack_->codecParms.extradataSize = opusHeadSize;
    return Status::OK;
}

AudioSampleFormat ConvertToLEFormat(AudioSampleFormat sampleFormat)
{
    switch (sampleFormat) {
        case AudioSampleFormat::SAMPLE_S16BE:
            return AudioSampleFormat::SAMPLE_S16LE;
        case AudioSampleFormat::SAMPLE_S24BE:
            return AudioSampleFormat::SAMPLE_S24LE;
        case AudioSampleFormat::SAMPLE_S32BE:
            return AudioSampleFormat::SAMPLE_S32LE;
        case AudioSampleFormat::SAMPLE_F32BE:
            return AudioSampleFormat::SAMPLE_F32LE;
        default:
            return sampleFormat; // No conversion needed
    }
}

Status MPEG4AtomParser::ParsePcmc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_PCMC_SIZE, Status::ERROR_INVALID_DATA, "Invalid pcmc atom size");
    ctx->offset += currentAtom.size;

    auto pcmcData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(pcmcData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate pcmc buffer");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, pcmcData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read pcmc data failed");

    constexpr size_t flagOffset = 4;
    uint8_t formatFlag = pcmcData[flagOffset];

    AudioSampleFormat sampleFormat = AudioSampleFormat::INVALID_WIDTH;
    mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    if (formatFlag == 1) {
        sampleFormat = ConvertToLEFormat(sampleFormat);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseEnda(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    ctx->offset += currentAtom.size;
    auto endaData = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(endaData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate enda buffer");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, endaData.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read enda data failed");
    uint16_t littleEndian = GetU16Value(&endaData[0]) & 0xFF;

    AudioSampleFormat sampleFormat = AudioSampleFormat::INVALID_WIDTH;
    mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    if (littleEndian == 1) {
        sampleFormat = ConvertToLEFormat(sampleFormat);
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
    }
    return Status::OK;
}

// 解析ICC Profile
Status MPEG4AtomParser::ParseIccProfile(int64_t offset, uint64_t dataSize)
{
    FALSE_RETURN_V_MSG_E(dataSize <= MAX_DATA_SIZE, Status::ERROR_INVALID_DATA,
        "Invalid ICC profile size");

    std::vector<uint8_t> iccProfile(dataSize);
    Status ret = dataReader_->ReadUintData(offset, iccProfile.data(), dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read ICC profile data failed");
    return ret;
}

Status MPEG4AtomParser::ParseColorParams(int64_t offset, uint64_t dataSize, const std::string& colorType,
    ParseContext* ctx)
{
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
    std::string mimeType;
    mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::MIME_TYPE>(mimeType);
    if (mimeType == MimeType::VIDEO_HEVC) {
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
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "No current track");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_COLOR_SIZE, Status::ERROR_INVALID_DATA, "colr atom too small");
    ctx->offset += currentAtom.size;

    uint8_t colorParamType[4];
    int32_t typeSize = sizeof(colorParamType);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, colorParamType, sizeof(colorParamType));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read color parameter type failed");
    std::string colorType(reinterpret_cast<char*>(colorParamType), sizeof(colorParamType));

    if (colorType != "nclx" && colorType != "nclc" && colorType != "prof") {
        MEDIA_LOG_W("Unsupported color parameter type: " PUBLIC_LOG_S, colorType.c_str());
        return Status::OK;
    }

    uint64_t dataSize = 0;
    if (currentAtom.dataSize > static_cast<uint64_t>(typeSize)) {
        dataSize = currentAtom.dataSize - typeSize;
    } else {
        return Status::ERROR_INVALID_DATA;
    }

    if (colorType == "prof") {
        // ICC Profile类型：剩余数据全部是profile
        ret = ParseIccProfile(ctx->dataOffset + typeSize, dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse ICC profile failed");
    } else {
        ret = ParseColorParams(ctx->dataOffset + typeSize, dataSize, colorType, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse color parameters failed");
    }
    return ret;
}

Status MPEG4AtomParser::ParseAclr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_ACLR_SIZE, Status::ERROR_INVALID_DATA,
        "aclr atom size too small");
    ctx->offset += currentAtom.size;
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
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize <= MAX_SIZE, Status::ERROR_INVALID_DATA, "glbl atom size too large");
    constexpr uint64_t minCheckSize = 10;
    // 检查是否嵌套fiel atom
    if (currentAtom.dataSize >= minCheckSize) {
        uint8_t header[8];
        constexpr uint32_t typeOffset = 4;
        Status ret = dataReader_->ReadUintData(ctx->dataOffset, header, sizeof(header));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read glbl header failed");
        uint32_t size = GetU32Value(&header[0]);
        uint32_t type = GetU32Value(&header[typeOffset]);
        if (type == FourccType("fiel") && size == currentAtom.dataSize) {
            MPEG4Atom fielAtom = currentAtom;
            fielAtom.type = type;
            fielAtom.size = size;
            fielAtom.dataSize = size - sizeof(header);
            ctx->offset = ctx->dataOffset;
            return ParseFiel(fielAtom, depth, ctx);
        }
    }
    // 避免重复extradata
    if (lastTrack_->codecParms.extradataSize > 1 && lastTrack_->codecParms.data) {
        MEDIA_LOG_W("ignoring multiple glbl");
        ctx->offset += currentAtom.size;
        return Status::OK;
    }
    // 读取glbl内容到extradata
    auto extradata = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, extradata.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read glbl extradata failed");
    lastTrack_->codecParms.data = std::move(extradata);
    lastTrack_->codecParms.extradataSize = currentAtom.dataSize;
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseStts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
        "Track or sampleHelper is nullptr");
    ctx->offset += currentAtom.size;
    if (currentAtom.size > MIN_BOX_SIZE) {
        Status ret = lastTrack_->sampleHelper->SetTimeToSampleParams(ctx->dataOffset, currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set time to sample params failed");
    } else {
        MEDIA_LOG_W("Stts atom size is too small, skipping time to sample parsing");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseStss(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
        "Track or sampleHelper is nullptr");

    ctx->offset += currentAtom.size;
    Status ret = lastTrack_->sampleHelper->SetSyncSampleParams(ctx->dataOffset, currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set sync sample params failed");
    return ret;
}

Status MPEG4AtomParser::ParseCtts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
        "Track or sampleHelper is nullptr");

    ctx->offset += currentAtom.size;
    Status ret = lastTrack_->sampleHelper->SetCompositionTimeToSampleParams(ctx->dataOffset, currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set composition time to sample params failed");
    return ret;
}

Status MPEG4AtomParser::ParseStsc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
        "Track or sampleHelper is nullptr");
    ctx->offset += currentAtom.size;
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
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
        "Track or sampleHelper is nullptr");
    ctx->offset += currentAtom.size;
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
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr && lastTrack_->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
        "Track or sampleHelper is nullptr");
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

Status MPEG4AtomParser::ParseFiel(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    ctx->offset += currentAtom.size;

    FieldOrder decodedFieldOrder = FieldOrder::FIELD_ORDER_UNKNOWN;
    uint8_t fieldOrder[2];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, fieldOrder, sizeof(fieldOrder));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read field order failed");
    uint16_t fieldOrderValue = GetU16Value(fieldOrder);
    if ((fieldOrderValue & 0xFF00) == 0X0100) {
        decodedFieldOrder = FieldOrder::FIELD_ORDER_PROGRESSIVE;
    } else if ((fieldOrderValue & 0xFF00) == 0X0200) {
        switch (fieldOrderValue & 0xFF) {
            case 0x01:
                decodedFieldOrder = FieldOrder::FIELD_ORDER_TT;
                break;
            case 0x06:
                decodedFieldOrder = FieldOrder::FIELD_ORDER_BB;
                break;
            case 0x09:
                decodedFieldOrder = FieldOrder::FIELD_ORDER_TB;
                break;
            case 0x0E:
                decodedFieldOrder = FieldOrder::FIELD_ORDER_BT;
                break;
            default:
                MEDIA_LOG_E("Invalid field order: " PUBLIC_LOG_U16, fieldOrderValue);
                return Status::ERROR_INVALID_DATA;
        }
    }
    if (decodedFieldOrder == FieldOrder::FIELD_ORDER_UNKNOWN && fieldOrderValue != 0) {
        MEDIA_LOG_E("Invalid field order: " PUBLIC_LOG_U16, fieldOrderValue);
        return Status::ERROR_INVALID_DATA;
    }
    return ret;
}

Status MPEG4AtomParser::ParseMvex(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_ONE, Status::ERROR_INVALID_PARAMETER, "Invalid mvex depth");
    Status ret = ParseContainerAtom(currentAtom, depth, ctx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse mvex atom failed");
    return ret;
}

Status MPEG4AtomParser::ParseTrex(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == MPEG4_DEPTH_TWO, Status::ERROR_INVALID_PARAMETER, "Invalid trex depth");
    ctx->offset += currentAtom.size;
    static uint32_t trexIndex = 0;
    // 根据索引找到对应的轨道
    auto track = firstTrack_;
    for (uint32_t i = 0; i < trexIndex && track != nullptr; ++i) {
        track = track->next;
    }
    if (track && track->sampleHelper) {
        Status ret = track->sampleHelper->SetTrackExtendsParams(ctx->dataOffset, currentAtom.dataSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set track extends params failed");
    } else {
        MEDIA_LOG_W("Track index " PUBLIC_LOG_U32 " not found or has no sampleHelper", trexIndex);
    }
    ++trexIndex;
    return Status::OK;
}

Status MPEG4AtomParser::ParseSidx(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(depth == 0, Status::ERROR_INVALID_DATA, "Invalid sidx depth");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_SIDX_SIZE, Status::ERROR_INVALID_DATA, "Invalid sidx atom size");

    if (isDistributedSidx_) {
        ctx->offset += currentAtom.size;
        MEDIA_LOG_W("Distributed sidx, skip parse");
        return Status::OK;
    }

    auto headerInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, headerInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sidx header info failed");
    uint8_t version = headerInfo[0];
    uint32_t referenceId = GetU32Value(&headerInfo[4]);
    auto currentTrack = firstTrack_;
    while (currentTrack && static_cast<uint32_t>(currentTrack->trackId) != referenceId) {
        currentTrack = currentTrack->next;
    }
    uint32_t timeScale = GetU32Value(&headerInfo[8]);
    FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_INVALID_DATA, "Invalid time scale: " PUBLIC_LOG_U32, timeScale);
    int64_t currentOffset = 12; // Skip version, flags, referenceId, and timeScale

    uint64_t firstOffset = 0;
    constexpr uint64_t firstOffsetV1 = 20;
    constexpr uint64_t firstOffsetV0 = 16;
    if (version == 0) {
        sidxCtx_.dts = GetU32Value(&headerInfo[currentOffset]);
        firstOffset = GetU32Value(&headerInfo[firstOffsetV0]);
        currentOffset += sizeof(uint64_t);
    } else if (version == 1) {
        sidxCtx_.dts = GetU64Value(&headerInfo[currentOffset]);
        firstOffset = GetU64Value(&headerInfo[firstOffsetV1]);
        currentOffset += sizeof(sidxCtx_.dts) + sizeof(firstOffset);
    }
    int64_t skipSize = 2;
    sidxCtx_.firstMoofOffset = ctx->offset + currentAtom.size + firstOffset;
    sidxCtx_.entryCount = GetU16Value(&headerInfo[currentOffset + skipSize]);
    FALSE_RETURN_V_MSG_E(sidxCtx_.entryCount > 0, Status::ERROR_INVALID_DATA, "Invalid sidx entry count");
    currentOffset += sizeof(uint32_t);
    constexpr uint32_t defaultEntrySize = 12;
    if (currentAtom.dataSize < sidxCtx_.entryCount * defaultEntrySize) {
        MEDIA_LOG_E("Invalid sidx data size: " PUBLIC_LOG_U64 ", expected at least " PUBLIC_LOG_U32,
            currentAtom.dataSize, sidxCtx_.entryCount * defaultEntrySize);
        return Status::ERROR_INVALID_DATA;
    }

    ret = ParseSidxEntries(headerInfo.get(), currentOffset, sidxCtx_, currentTrack);
    MEDIA_LOG_D("current track index" PUBLIC_LOG_U32 " sidx duration: " PUBLIC_LOG_U64,
        currentTrack->trackIndex, currentTrack->sidxDuration);
    ctx->offset += currentAtom.size;
    hasSidxBox_ = true;
    return ret;
}

Status MPEG4AtomParser::ParseSidxEntries(const uint8_t* headerInfo, int64_t entryOffset,
    SidxParseContext& sidxCtx, std::shared_ptr<Track> currentTrack)
{
    constexpr uint32_t defaultEntrySize = 12;
    for (uint32_t i = 0; i < sidxCtx.entryCount; ++i) {
        uint32_t size = GetU32Value(&headerInfo[entryOffset + defaultEntrySize * i]);
        uint32_t duration = GetU32Value(&headerInfo[entryOffset + 4 + defaultEntrySize * i]);
        if (size & 0x80000000) {
            MEDIA_LOG_W("Sub sidx not supported");
        }
        MEDIA_LOG_D("Entry " PUBLIC_LOG_U32 ": offset=" PUBLIC_LOG_D64 ", size="
            PUBLIC_LOG_U32 ", duration=" PUBLIC_LOG_U32, i, sidxCtx.firstMoofOffset, size, duration);
        int64_t firstMoofOffset = sidxCtx.firstMoofOffset;
        bool found = std::any_of(fragmentEntry_.begin(), fragmentEntry_.end(),
            [firstMoofOffset](const FragmentEntry& entry) {
                return entry.moofOffset == firstMoofOffset;
            });
        if (!found) {
            fragmentEntry_.emplace_back(FragmentEntry {
                .firstDts = static_cast<int64_t>(sidxCtx.dts),
                .moofOffset = sidxCtx.firstMoofOffset,
                .duration = static_cast<int64_t>(duration),
                .hasRead = false
            });
        }
        if (currentTrack) {
            currentTrack->sidxDuration += duration;
        }
        sidxCtx.firstMoofOffset += size;
        sidxCtx.dts += duration;
    }
    return Status::OK;
}

Status MPEG4AtomParser::FindNextMoof(int64_t& headerOffset, uint32_t& atomSize)
{
    Status ret = Status::OK;
    int32_t atomType = 0;
    uint32_t typeOffset = 4;
    do {
        auto atomHeader = std::make_unique<uint8_t[]>(0x08);
        FALSE_RETURN_V_MSG_E(atomHeader != nullptr, Status::ERROR_NO_MEMORY, "atomHeader is nullptr");
        ret = dataReader_->ReadUintData(headerOffset, atomHeader.get(), 0x08);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Find next moof header info failed");
        atomSize = GetU32Value(atomHeader.get());
        atomType = static_cast<int32_t>(GetU32Value(atomHeader.get() + typeOffset));
        if (atomType != FourccType("moof")) {
            MEDIA_LOG_D("not moof box, go find next");
            auto atomData = std::make_unique<uint8_t[]>(atomSize - sizeof(atomHeader));
            FALSE_RETURN_V_MSG_E(atomData != nullptr, Status::ERROR_NO_MEMORY, "atomHeader is nullptr");
            ret = dataReader_->ReadUintData(headerOffset + sizeof(atomHeader), atomData.get(),
                atomSize - sizeof(atomHeader));
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
        if (GetU32Value(&atomHeader[0x04]) == FourccType("sidx")) {
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
        case FourccType("mfhd"): {
            uint8_t mfhdData[atom.dataSize];
            ret = dataReader_->ReadUintData(atom.dataOffset, mfhdData, sizeof(mfhdData));
            FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Read data failed");
            currentOffset += atom.size;
            break;
        }
        case FourccType("traf"): {
            currentOffset += atom.headerSize;
            break;
        }
        case FourccType("tfhd"): {
            currentOffset += atom.size;
            ret = ProcessTfhdAtom(atom);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Process tfhd atom failed");
            break;
        }
        case FourccType("tfdt"): {
            currentOffset += atom.size;
            ret = currentTrack_->sampleHelper->SetTrackFragmentDecodeTimeParams(atom.dataOffset, atom.dataSize);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Set track fragment decode time params failed");
            break;
        }
        case FourccType("trun"): {
            currentOffset += atom.size;
            int64_t moofDts = 0;
            int64_t moofDuration = 0;
            ret = currentTrack_->sampleHelper->SetTrackFragmentRunParams(atom.dataOffset, atom.dataSize, sidxDts,
                moofDts, moofDuration);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Set track run params failed");
            frag.firstDts = (frag.firstDts == -1) ? moofDts : frag.firstDts;
            frag.duration = (frag.duration == -1) ? moofDuration : frag.duration;
            break;
        }
        default: {
            MEDIA_LOG_W("Unknown atom type: " PUBLIC_LOG_S, FourccToString(atom.type).c_str());
            ret = Status::ERROR_INVALID_DATA;
            break;
        }
    }
    return true;
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
        // 如果已经有moof信息，直接返回
        auto it = std::find_if(fragmentEntry_.begin(), fragmentEntry_.end(),
            [this](const FragmentEntry& entry) { return entry.moofOffset == moofOffset_; });
        if (it != fragmentEntry_.end()) {
            inEntry = true;
            fragIndex = std::distance(fragmentEntry_.begin(), it);
            if (it->hasRead) {
                MEDIA_LOG_D("Moof already parsed at offset: " PUBLIC_LOG_D64, moofOffset_);
                return Status::OK;
            }
        }
    }
    int64_t sidxDts = inEntry ? fragmentEntry_[fragIndex].firstDts : -1;
    firstTrack_ = (firstTrack_ == nullptr) ? track : firstTrack_;
    uint32_t typeOffset = 4;
    int64_t moofEndOffset = moofOffset_ + atomSize;
    int64_t currentOffset = moofOffset_ + sizeof(uint64_t); // Skip size and type
    currentTrack_ = firstTrack_;
    while (currentOffset < moofEndOffset) {
        MPEG4Atom atom;
        uint8_t atomHeader[8];
        atom.headerSize = sizeof(atomHeader);
        ret = dataReader_->ReadUintData(currentOffset, atomHeader, atom.headerSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read moof atom header failed");
        atom.size = GetU32Value(&atomHeader[0]);
        atom.type = static_cast<int32_t>(GetU32Value(&atomHeader[typeOffset]));
        FALSE_RETURN_V_MSG_E(
            currentOffset + static_cast<int64_t>(atom.size) <= moofEndOffset, Status::ERROR_INVALID_DATA,
            "Invalid moof size: " PUBLIC_LOG_U64 ", moof end offset: " PUBLIC_LOG_D64, atom.size, moofEndOffset);
        atom.dataSize = atom.size - atom.headerSize;
        atom.dataOffset = currentOffset + atom.headerSize; // Skip size and type
        MEDIA_LOG_D("Processing box: " PUBLIC_LOG_S ", size: " PUBLIC_LOG_U64 " at offset " PUBLIC_LOG_D64,
            FourccToString(atom.type).c_str(), atom.dataSize, atom.dataOffset);
        FALSE_RETURN_V_NOLOG(UpdataMoofInfo(atom, ret, currentOffset, frag, sidxDts), ret);
    }
    ret = UpdateFragmentEntryAndOffset(inEntry, fragIndex, frag, moofOffset_, currentOffset);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Update fragment entry and offset failed");
    return Status::OK;
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
        return Status::OK;
    }
    
    int64_t tfhdDataOffset = atom.dataOffset + sizeof(tfhdHeader);
    int64_t dataSize = static_cast<int64_t>(atom.dataSize - sizeof(tfhdHeader));
    ret = currentTrack_->sampleHelper->SetTrackFragmentHeaderParams(tfhdDataOffset, dataSize, tfhdFlag, moofOffset_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Set track fragment header params failed");
    
    return Status::OK;
}

Status MPEG4AtomParser::ParseUdta(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
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
    if (static_cast<uint32_t>(std::distance(numbers, end)) >= minLocationNum) {
        float latitude = std::stof(numbers->str());
        float longitude = std::stof((++numbers)->str());

        mediaInfo_.general.Set<Tag::MEDIA_LATITUDE>(latitude);
        mediaInfo_.general.Set<Tag::MEDIA_LONGITUDE>(longitude);
    } else {
        MEDIA_LOG_W("Invalid location format: " PUBLIC_LOG_S, locationStr.c_str());
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

Status MPEG4AtomParser::ParseCover(MPEG4Atom currentAtom, uint32_t dataType, uint64_t stringSize, ParseContext* ctx)
{
    std::string mimeType = GetCoverMimeType(dataType);
    if (mimeType.empty()) {
        return Status::OK; // 不支持的封面类型，直接返回
    }

    auto coverData = std::make_unique<uint8_t[]>(stringSize);
    FALSE_RETURN_V_MSG_E(coverData != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate cover data buffer");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset + 16, coverData.get(), stringSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read cover data failed");

    auto coverTrack = std::make_shared<Track>();
    FALSE_RETURN_V_MSG_E(coverTrack != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate cover track");
    
    coverTrack->trackIndex = trackCount_;
    coverTrack->codecParms.trackType = ATTACHMENT_TYPE;
    coverTrack->codecParms.extradataSize = stringSize;

    Meta coverMeta;
    coverMeta.Set<Tag::MIME_TYPE>(mimeType);

    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    auto currentTrack = firstTrack_;
    while (currentTrack) {
        if (currentTrack->codecParms.trackType == VIDEO_TYPE &&
            IsValidTrackIndex(mediaInfo_, currentTrack->trackIndex)) {
            if (mediaInfo_.tracks[currentTrack->trackIndex].Get<Tag::VIDEO_WIDTH>(videoWidth) &&
                mediaInfo_.tracks[currentTrack->trackIndex].Get<Tag::VIDEO_HEIGHT>(videoHeight)) {
                break;
            }
        }
        currentTrack = currentTrack->next;
    }
    std::vector<uint8_t> cover(stringSize);
    cover.assign(coverData.get(), coverData.get() + stringSize);
    coverMeta.Set<Tag::MEDIA_COVER>(std::move(cover));
    mediaInfo_.tracks.emplace_back(std::move(coverMeta));
    
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
    ctx->offset += currentAtom.size;
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
                MEDIA_LOG_D("Found dynamic key[" PUBLIC_LOG_U32 "]: " PUBLIC_LOG_S, index, key.c_str());
                return key;
            }
        } else if (atomType != (FourccType("covr"))) {
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
    if (dataTag == FourccType("data") && dataSize <= currentAtom.dataSize && dataSize >= headerSize) {
        uint8_t typeInfo[8];
        ret = dataReader_->ReadUintData(ctx->dataOffset + sizeof(typeInfo), typeInfo, sizeof(typeInfo));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data type info failed");
        
        uint32_t dataType = GetU32Value(&typeInfo[0]);
        uint64_t stringSize = dataSize - headerSize;
        if (currentAtom.type == FourccType("covr") || (!key.empty() && key == "com.apple.quicktime.artwork")) {
            return ParseCover(currentAtom, dataType, stringSize, ctx);
        }
        ret = ReadMetadataString(key, ctx->dataOffset + headerSize, stringSize, dataType);
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
        ret = ReadMetadataString(key, ctx->dataOffset, currentAtom.dataSize, 0);
    } else {
        // 跳过语言代码
        ret = ReadMetadataString(key, ctx->dataOffset + defaultSkipSize, stringLength, 0);
    }
    
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read traditional metadata string failed");
    return Status::OK;
}

int32_t ParseSignedInteger(uint8_t* data, uint64_t size)
{
    FALSE_RETURN_V_MSG_E(data != nullptr, 0, "ParseSignedInteger: data is null");
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
            if (value & 0x800000) {
                value |= 0xFF000000;
            }
            break;
        case 0x04:
            value = static_cast<int32_t>(GetU32Value(data));
            break;
        default:
            value = 0;
            break;
    }
    return value;
}

std::string DecodeMacString(const uint8_t* data, size_t size)
{
    FALSE_RETURN_V_MSG_E(data != nullptr || size > 0, "", "DecodeMacString: data is null");
    
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

bool MPEG4AtomParser::SetMetadataValue(const std::string& key, const std::string& value)
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
    };

    auto it = keyToTagMap.find(Converter::ToLower(key));
    if (it != keyToTagMap.end()) {
        std::string resultStr = "";
        if (!Converter::IsUTF8(value.c_str()) && Converter::IsGBK(value.c_str())) {
            resultStr = Converter::ConvertGBKToUTF8(value);
        }
        resultStr = resultStr.length() > 0 ? resultStr : value;
        if (resultStr.length() > 0) {
            mediaInfo_.general.SetData(it->second, resultStr);
        }
        return true;
    } else if (key == "location") {
        ParseLocationString(value);
        return true;
    }
    return false;
}

Status MPEG4AtomParser::ReadMetadataString(std::string& key, int64_t offset, uint64_t size, uint32_t dataType)
{
    FALSE_RETURN_V_NOLOG(size > 0, Status::OK);
    auto buffer = std::make_unique<uint8_t[]>(size + 1);
    FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate metadata buffer");
    Status ret = dataReader_->ReadUintData(offset, buffer.get(), size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read metadata string data failed");
    
    buffer[size] = '\0';
    if (SetMetadataValue(key, std::string(reinterpret_cast<char*>(buffer.get()), size)) &&
        !UserMetaToSource(Converter::ToLower(key))) {
        return Status::OK;
    }
    FALSE_RETURN_V_NOLOG(userFormat_ != nullptr, Status::ERROR_NO_MEMORY);
    switch (dataType) {
        case 0x00: { // reserved buffer
            std::vector<uint8_t> bufferValue(buffer.get(), buffer.get() + size);
            userFormat_->SetData(key, bufferValue);
            break;
        }
        case 0x15:
        case 0x43: { // BE signed integer
            userFormat_->SetData(key, ParseSignedInteger(buffer.get(), size));
            break;
        }
        case 0x17: { // BE float32
            if (size >= MIN_FLOAT_SIZE) {
                uint32_t uintValue = GetU32Value(buffer.get());
                float floatValue = *reinterpret_cast<float*>(&uintValue);
                userFormat_->SetData(key, floatValue);
            }
            break;
        }
        case 0x03: { // MAC encoded
            std::string macStringValue = DecodeMacString(buffer.get(), size);
            if (!macStringValue.empty()) {
                userFormat_->SetData(key, macStringValue);
            }
            break;
        }
        default: { // UTF-8 string
            std::string stringValue = std::string(reinterpret_cast<char*>(buffer.get()), size);
            if (!stringValue.empty()) {
                userFormat_->SetData(key, stringValue);
            }
            break;
        }
    }
    
    return Status::OK;
}

Status MPEG4AtomParser::ParseMeta(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    int64_t endOffset = 0;
    if (__builtin_add_overflow(ctx->offset, currentAtom.size, &endOffset)) {
        MEDIA_LOG_E("Container atom size overflow");
        return Status::ERROR_INVALID_DATA;
    }
    if (currentAtom.dataSize < sizeof(uint32_t) || depth == MPEG4_ROOT_DEPTH) {
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
    if (firstType == FourccType("hdlr") && firstSize > sizeof(testBytes) && firstSize <= currentAtom.dataSize) {
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
    ctx->offset = endOffset;
    return Status::OK;
}

Status MPEG4AtomParser::ParseKeys(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= DEFAULT_HEAD_SIZE, Status::ERROR_INVALID_DATA, "Invalid atom size");
    ctx->offset += currentAtom.size;
    // skip version and flags (4 bytes)
    auto keysInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, keysInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read keys atom info failed");
    uint32_t keysCount = GetU32Value(&keysInfo[4]);
    MEDIA_LOG_D("Parsing keys atom with count: " PUBLIC_LOG_U32, keysCount);
    if (keysCount > UINT_MAX / sizeof(std::string) - 1) {
        MEDIA_LOG_E("The 'keys' atom with the invalid key count: " PUBLIC_LOG_U32, keysCount);
        return Status::ERROR_INVALID_DATA;
    }
    // 预分配空间，索引从1开始，0不使用
    ctx->metaKeys.resize(keysCount + 1);
    ctx->metaKeysCount = keysCount + 1;
    // 解析keys
    int64_t keysOffset = 8;
    for (uint32_t i = 1; i <= keysCount; ++i) {
        uint32_t keySize = GetU32Value(&keysInfo[keysOffset]);
        uint32_t keyType = GetU32Value(&keysInfo[keysOffset + sizeof(uint32_t)]);
        if (keySize < sizeof(uint64_t)) {
            MEDIA_LOG_E("Invalid key size: " PUBLIC_LOG_U32, keySize);
            return Status::ERROR_INVALID_DATA;
        }
        keysOffset += sizeof(uint64_t);
        uint32_t strLen = keySize - sizeof(uint64_t);
        if (keyType != FourccType("mdta")) {
            keysOffset += strLen;
            continue;
        }
        if (strLen > 0) {
            std::string keyStr(reinterpret_cast<char*>(&keysInfo[keysOffset]), strLen);
            // 确保字符串以null结尾
            keyStr = keyStr.substr(0, keyStr.find('\0'));
            ctx->metaKeys[i] = keyStr;
        } else {
            MEDIA_LOG_E("Invalid key string length: " PUBLIC_LOG_U32, strLen);
            return Status::ERROR_INVALID_DATA;
        }
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
    ctx->offset += currentAtom.size;

    auto lociInfo = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, lociInfo.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read loci atom info failed");

    uint16_t languageCode = GetU16Value(&lociInfo[4]);
    std::string language = ConvertLanguageCode(languageCode);
    MEDIA_LOG_D("Parsing loci atom with language: " PUBLIC_LOG_S, language.c_str());

    uint64_t offset = 6; // 跳过version+flags(4) + language(2)
    uint64_t remainingLen = currentAtom.dataSize - offset;
    
    // 跳过地点名称字符串（以null结尾）
    uint64_t placeStartOffset = offset;
    while (offset < currentAtom.dataSize && lociInfo[offset] != 0) {
        ++offset;
    }
    
    FALSE_RETURN_V_MSG_E(offset < currentAtom.dataSize, Status::ERROR_INVALID_DATA,
        "loci place name not null-terminated");
    // 提取地点名称
    std::string place;
    if (offset > placeStartOffset) {
        place = std::string(reinterpret_cast<char*>(&lociInfo[placeStartOffset]),
                            offset - placeStartOffset);
    }
    ++offset; // 跳过null terminator
    remainingLen = currentAtom.dataSize - offset;
    // 检查剩余数据是否足够（role(1) + coordinates(12) = 13字节）
    if (remainingLen < 13) {
        MEDIA_LOG_E("loci too short (" PUBLIC_LOG_U64 " bytes left, need at least 13)", remainingLen);
        return Status::ERROR_INVALID_DATA;
    }
    ++offset; // 跳过role (1字节)

    int32_t longitudeRaw = static_cast<int32_t>(GetU32Value(&lociInfo[offset]));
    int32_t latitudeRaw = static_cast<int32_t>(GetU32Value(&lociInfo[offset + 4]));
    float longitude = longitudeRaw / static_cast<float>(CONVERT_SCALE);
    float latitude = latitudeRaw / static_cast<float>(CONVERT_SCALE);

    constexpr int32_t precision = 4;
    constexpr int32_t latitudeWidth = 8;
    constexpr int32_t longitudeWidth = 9;
    std::ostringstream locationStream;
    locationStream << std::fixed <<
                      std::setprecision(precision) <<
                      std::showpos <<
                      std::setfill('0') <<
                      std::setw(latitudeWidth) << latitude <<
                      std::setw(longitudeWidth) << longitude;

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

Status MPEG4AtomParser::ParseUdtaGenre(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
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

Status MPEG4AtomParser::ParseMoovGenre(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    if (currentAtom.size == 0) {
        return Status::OK;
    }
    ctx->offset += currentAtom.size;
    auto genre = std::make_unique<uint8_t[]>(currentAtom.size + 1);
    FALSE_RETURN_V_MSG_E(genre != nullptr, Status::ERROR_NO_MEMORY, "Read moov genre failed");
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, genre.get(), currentAtom.size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");

    int32_t start = 0;
    for (start = 0; start < currentAtom.size; ++start) {
        if (genre[start] != 0) {
            break;
        }
    }
    SetMetadataValue("genre", std::string(reinterpret_cast<char*>(genre.get() + start)));
    return Status::OK;
}

Status MPEG4AtomParser::ParseLabl(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseFrma(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    auto info = std::make_unique<uint8_t[]>(currentAtom.dataSize);
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, info.get(), currentAtom.dataSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read frma atom info failed");
    uint32_t originalFormat = GetU32Value(info.get());
    std::string originalFormatStr = FourccToString(originalFormat);
    MEDIA_LOG_D("Parsing frma atom with original format: " PUBLIC_LOG_S, originalFormatStr.c_str());
    if (!isIsom_ && originalFormat == FourccType("alac")) {
        ret = ParseAlacEntry(*ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse alac entry failed");
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseSchi(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseSchm(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseKind(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::ParseChan(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    FALSE_RETURN_V_MSG_E(lastTrack_ != nullptr, Status::ERROR_NULL_POINTER, "Current track is null");
    FALSE_RETURN_V_MSG_E(currentAtom.dataSize >= MIN_BOX_SIZE, Status::ERROR_INVALID_DATA, "Chan corrupted");
    const uint32_t chanEntrySize = 20;
    ctx->offset += currentAtom.size;
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
    FALSE_RETURN_V(currentAtom.dataSize >= entryOffset + entryNum * chanEntrySize, Status::OK);

    for (uint32_t i = 0; i < entryNum; ++i) {
        uint32_t channelLabel = GetU32Value(&chanData[currentOffset]);
        currentOffset += chanEntrySize;
        if (layoutTag == 0) {
            int64_t channelSet = ReadChannelSet(channelLabel);
            if (channelSet == 0) {
                channelSets = 0;
                break;
            }
            channelSets |= channelSet;
        }
    }
    channelLayoutMask = layoutTag == 0 ? (channelSets ? channelSets : 0) : GetChannelLayoutByBitmap(layoutTag, bitmap);
    if (channelLayoutMask) {
        auto audioParser = std::make_shared<MPEG4AudioParser>();
        FALSE_RETURN_V_MSG_E(audioParser != nullptr, Status::ERROR_NO_MEMORY, "Failed create audioParser");
        AudioChannelLayout layout = audioParser->FindValidChannelLayout(channelLayoutMask);
        if (layout == AudioChannelLayout::UNKNOWN) {
            int32_t channelCount = 0;
            FALSE_RETURN_V_MSG_E(
                mediaInfo_.tracks[lastTrack_->trackIndex].Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount),
                Status::ERROR_INVALID_DATA, "Channel count missed");
            layout = audioParser->GetDefaultChannelLayout(channelCount);
        }
        mediaInfo_.tracks[lastTrack_->trackIndex].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
    }
    return Status::OK;
}

Status MPEG4AtomParser::ParseChap(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    return Status::OK;
}

Status MPEG4AtomParser::MPEG4ParseHeader(const std::shared_ptr<DataSource>& source, Seekable seekable)
{
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");
    dataSource_ = source;
    seekable_ = seekable;
    dataReader_ = std::make_shared<DemuxerDataReader>();
    FALSE_RETURN_V_MSG_E(dataReader_ != nullptr, Status::ERROR_NO_MEMORY, "Failed create dataReader");
    dataReader_->SetDataReader(source);
    Status ret = Status::OK;
    uint64_t dataSourceSize = 0;
    Status sizeRet = dataSource_->GetSize(dataSourceSize);
    /*
     * Unseekable时,解析完moov和找到mdat就退出解析
     * Seekable时,存在sidx box时只解析到第一个moof为止;若存在分散的sidx则解析完整个文件
     * Seekable时,没有sidx的fmp4解析所有moof，普通MPEG4解析到文件末尾
    */
    while ((seekable == Seekable::UNSEEKABLE && !(moovFound_ && hasMdatBox_)) ||
           (seekable == Seekable::SEEKABLE && (!(hasMoofBox_ && hasSidxBox_) || isDistributedSidx_)) ||
           (seekable == Seekable::SEEKABLE && hasMoofBox_ && !hasSidxBox_)) {
        if (sizeRet == Status::OK && ctx_.offset >= static_cast<int64_t>(dataSourceSize)) {
            MEDIA_LOG_D("Reached end of data source at offset: " PUBLIC_LOG_D64, ctx_.offset);
            break;
        }
        int64_t originalOffset = ctx_.offset;
        int32_t depth = 0;
        ret = MPEG4ParseAtom(depth, &ctx_);
        if (ret != Status::OK) {
            MEDIA_LOG_E("Parse error occurred: " PUBLIC_LOG_D32, static_cast<int32_t>(ret));
            break;
        } else if (ctx_.offset <= originalOffset) {
            MEDIA_LOG_E("Parse header failed");
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    FALSE_RETURN_V_MSG_E(moovFound_, Status::ERROR_INVALID_DATA, "MPEG4 parse failed");
    CalculateMPEG4Attributes();
    return ret;
}

Status MPEG4AtomParser::ParseClli(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    FALSE_RETURN_V_MSG_D(currentAtom.size >= MIN_CLLI_SIZE, Status::OK, "Clli: Empty");
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
    lastTrack_->codecParms.hdrBoxInfo.isHdr = true;
    return Status::OK;
}

Status MPEG4AtomParser::ParseMdcv(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx)
{
    ctx->offset += currentAtom.size;
    FALSE_RETURN_V_MSG_D(currentAtom.size >= MIN_MDCV_SIZE, Status::OK, "Mdcv: Empty");
    uint8_t info[24];
    Status ret = dataReader_->ReadUintData(ctx->dataOffset, info, sizeof(info));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read chan data failed");
    float displayPrimaryRedX = static_cast<float>(GetU16Value(&info[0]));
    float displayPrimaryRedY = static_cast<float>(GetU16Value(&info[2]));
    float displayPrimaryGreenX = static_cast<float>(GetU16Value(&info[4]));
    float displayPrimaryGreenY = static_cast<float>(GetU16Value(&info[6]));
    float displayPrimaryBlueX = static_cast<float>(GetU16Value(&info[8]));
    float displayPrimaryBlueY = static_cast<float>(GetU16Value(&info[10]));
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
    lastTrack_->codecParms.hdrBoxInfo.isHdr = true;
    return Status::OK;
}

void MPEG4AtomParser::InitParseTable()
{
    // 填充解析函数表
    MPEG4ParseTable_ = {
        {FourccType("ftyp"), &MPEG4AtomParser::ParseFtyp}, {FourccType("moov"), &MPEG4AtomParser::ParseMoov},
        {FourccType("wide"), &MPEG4AtomParser::ParseWide}, {FourccType("mdat"), &MPEG4AtomParser::ParseMdat},
        {FourccType("trak"), &MPEG4AtomParser::ParseTrak}, {FourccType("mvhd"), &MPEG4AtomParser::ParseMvhd},
        {FourccType("tkhd"), &MPEG4AtomParser::ParseTkhd}, {FourccType("edts"), &MPEG4AtomParser::ParseEdts},
        {FourccType("elst"), &MPEG4AtomParser::ParseElst}, {FourccType("tref"), &MPEG4AtomParser::ParseTref},
        {FourccType("cdsc"), &MPEG4AtomParser::ParseCdsc}, {FourccType("mdia"), &MPEG4AtomParser::ParseMdia},
        {FourccType("mdhd"), &MPEG4AtomParser::ParseMdhd}, {FourccType("hdlr"), &MPEG4AtomParser::ParseHdlr},
        {FourccType("minf"), &MPEG4AtomParser::ParseMinf}, {FourccType("stbl"), &MPEG4AtomParser::ParseStbl},
        {FourccType("stsd"), &MPEG4AtomParser::ParseStsd}, {FourccType("btrt"), &MPEG4AtomParser::ParseBtrt},
        {FourccType("pasp"), &MPEG4AtomParser::ParsePasp}, {FourccType("esds"), &MPEG4AtomParser::ParseEsds},
        {FourccType("avcC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("hvcC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("vvcC"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("d263"), &MPEG4AtomParser::ParseCodecConfig},
        {FourccType("dfLa"), &MPEG4AtomParser::ParseDfla}, {FourccType("dOps"), &MPEG4AtomParser::ParseDops},
        {FourccType("pcmC"), &MPEG4AtomParser::ParsePcmc}, {FourccType("enda"), &MPEG4AtomParser::ParseEnda},
        {FourccType("colr"), &MPEG4AtomParser::ParseColr}, {FourccType("aclr"), &MPEG4AtomParser::ParseAclr},
        {FourccType("glbl"), &MPEG4AtomParser::ParseGlbl}, {FourccType("stts"), &MPEG4AtomParser::ParseStts},
        {FourccType("stss"), &MPEG4AtomParser::ParseStss}, {FourccType("ctts"), &MPEG4AtomParser::ParseCtts},
        {FourccType("stsc"), &MPEG4AtomParser::ParseStsc}, {FourccType("stsz"), &MPEG4AtomParser::ParseStsz},
        {FourccType("stz2"), &MPEG4AtomParser::ParseStsz}, {FourccType("stco"), &MPEG4AtomParser::ParseStco},
        {FourccType("co64"), &MPEG4AtomParser::ParseStco}, {FourccType("fiel"), &MPEG4AtomParser::ParseFiel},
        {FourccType("mvex"), &MPEG4AtomParser::ParseMvex}, {FourccType("trex"), &MPEG4AtomParser::ParseTrex},
        {FourccType("sidx"), &MPEG4AtomParser::ParseSidx}, {FourccType("udta"), &MPEG4AtomParser::ParseUdta},
        {FourccType("meta"), &MPEG4AtomParser::ParseMeta}, {FourccType("ilst"), &MPEG4AtomParser::ParseIlst},
        {FourccType("loci"), &MPEG4AtomParser::ParseLoci}, {FourccType("labl"), &MPEG4AtomParser::ParseLabl},
        {FourccType("wave"), &MPEG4AtomParser::ParseWave}, {FourccType("frma"), &MPEG4AtomParser::ParseFrma},
        {FourccType("schi"), &MPEG4AtomParser::ParseSchi}, {FourccType("schm"), &MPEG4AtomParser::ParseSchm},
        {FourccType("kind"), &MPEG4AtomParser::ParseKind}, {FourccType("chan"), &MPEG4AtomParser::ParseChan},
        {FourccType("chap"), &MPEG4AtomParser::ParseChap}, {FourccType("gnre"), &MPEG4AtomParser::ParseMoovGenre},
        {FourccType("clli"), &MPEG4AtomParser::ParseClli}, {FourccType("mdcv"), &MPEG4AtomParser::ParseMdcv},
    };
}

Status MPEG4AtomParser::ParseAtomHeader(MPEG4Atom& atom, std::string& typeStr, int32_t depth,
    bool &isZeroSizeAtom, ParseContext* ctx)
{
    uint8_t atomInfo[8];
    const uint32_t typeOffset = 4; // type在第5-8字节
    Status ret = dataReader_->ReadUintData(ctx->offset, atomInfo, sizeof(atomInfo));
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read atom header info failed");
    
    atom.size = GetU32Value(&atomInfo[0]);
    atom.type = static_cast<int32_t>(GetU32Value(&atomInfo[typeOffset]));
    typeStr = FourccToString(atom.type);
    ctx->dataOffset = ctx->offset + sizeof(atomInfo);

    const uint64_t minExtendSize = 16;
    if (atom.size == 1) {
        uint8_t sizeData[8];
        ret = dataReader_->ReadUintData(ctx->dataOffset, sizeData, sizeof(sizeData));
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read atom extended size failed");
        atom.size = GetU64Value(sizeData);
        ctx->dataOffset += sizeof(sizeData);
        if (atom.size < minExtendSize) {
            MEDIA_LOG_E("Invalid extended atom size: " PUBLIC_LOG_U64, atom.size);
            return Status::ERROR_INVALID_DATA;
        }
    } else if (atom.size == 0) {
        if (depth == 0) { // 根级atoms
            uint64_t dataSourceSize = 0;
            ret = dataSource_->GetSize(dataSourceSize);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_INVALID_PARAMETER, "Failed to get data source size");
            atom.size = dataSourceSize - ctx->offset;
        } else {
            ctx->offset += sizeof(uint32_t); // 跳过size字段
            isZeroSizeAtom = true;
            return Status::OK;
        }
    }
    FALSE_RETURN_V_MSG_E(atom.size >= sizeof(atomInfo), Status::ERROR_INVALID_DATA, "Invalid atom size");
    return Status::OK;
}

void MPEG4AtomParser::SetHdrTypeInfo(MPEG4Atom currentAtom)
{
    FALSE_RETURN_NOLOG(lastTrack_ != nullptr);
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

    int64_t atomEndOffset = -1LL;
    if (__builtin_add_overflow(ctx->offset, atom.size, &atomEndOffset)) {
        MEDIA_LOG_E("Atom size overflow at offset: " PUBLIC_LOG_D64, ctx->offset);
        return Status::ERROR_INVALID_DATA;
    }
    PathAdder pathAdder(&ctx->path, atom.type);
    atom.dataSize = atom.size - (ctx->dataOffset - ctx->offset);
    FALSE_RETURN_V_MSG_E(atom.dataSize >= 0, Status::ERROR_INVALID_DATA, "Invalid atom data size");
    if (atom.type == FourccType("moof") && (isDistributedSidx_ || !hasSidxBox_ || !hasMoofBox_)) {
        ret = ParseMoof(firstTrack_, ctx->offset);
    }
    // 使用函数指针查找和调用解析函数
    ParseFunction parseFunc = FindAtomParser(atom, ctx);
    if (parseFunc) {
        ret = (this->*parseFunc)(atom, depth, ctx);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse atom failed for type: " PUBLIC_LOG_S, atomTypeStr.c_str());
    } else {
        // 无法识别的atom类型，跳过
        SetHdrTypeInfo(atom);
        ctx->offset += atom.size;
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
        if (parentType == FourccType("udta") || parentType == FourccType("ilst")) {
            if (atom.type == FourccType("gnre")) {
                return &MPEG4AtomParser::ParseUdtaGenre;
            } else {
                return &MPEG4AtomParser::ParseUserDataString;
            }
        } else if (parentType == FourccType("meta") && atom.type == FourccType("keys") &&
                ctx->foundHdlrMdta && ctx->metaKeysCount == 0) {
            return &MPEG4AtomParser::ParseKeys;
        }
    }
    return nullptr;
}

void MPEG4AtomParser::CalculateMPEG4Attributes()
{
    MEDIA_LOG_D("In");
    for (auto track = firstTrack_; track; track = track->next) {
        if (track->codecParms.trackType == VIDEO_TYPE) {
            CalculateVideoFrameRate(track);
            mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_DELAY>(0);
        }
        if (track->codecParms.trackType != ATTACHMENT_TYPE) {
            CalculateTrackBitrates(track);
        }
        SetTrackType(track);
    }

    SetStartTime();
    SetFileDuration();
    SetHEVCDefaultValue();
    FileType fileType;
    if (!mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType)) {
        MEDIA_LOG_D("MEDIA_FILE_TYPE not found, set default value");
        mediaInfo_.general.Set<Tag::MEDIA_FILE_TYPE>(FileType::MP4);  // 设置MEDIA_FILE_TYPE默认值
    }
}

void MPEG4AtomParser::SetTrackType(std::shared_ptr<Track> track)
{
    TagType tagType;
    switch (track->codecParms.trackType) {
        case VIDEO_TYPE:
            tagType = Tag::MEDIA_HAS_VIDEO;
            break;
        case AUDIO_TYPE:
            tagType = Tag::MEDIA_HAS_AUDIO;
            break;
        case SUBTITLE_TYPE:
            tagType = Tag::MEDIA_HAS_SUBTITLE;
            break;
        case TIMEDMETA_TYPE:
            tagType = Tag::MEDIA_HAS_TIMEDMETA;
            break;
        default:
            MEDIA_LOG_W("Unknown track type for track " PUBLIC_LOG_U32, track->trackIndex);
            return; // 不处理未知类型
    }
    mediaInfo_.general.SetData(tagType, true);
}

void MPEG4AtomParser::SetFileDuration()
{
    int64_t fileDuration = 0;
    if (mediaInfo_.general.Get<Tag::MEDIA_DURATION>(fileDuration) && fileDuration > 0) {
        return; // 已经设置了有效的duration，直接返回
    }
    if (!hasMoofBox_) {
        fileDuration = GetMpeg4FileDuration();
    } else {
        int64_t maxDuration = 0;
        uint32_t maxDurationTrackIndex = 0;
        fileDuration = FindMaxTrackDuration(maxDuration, maxDurationTrackIndex);
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
    mediaInfo_.general.Get<Tag::MEDIA_CONTAINER_START_TIME>(containerStartTime);
    MEDIA_LOG_D("Container start time: " PUBLIC_LOG_D64 " us", containerStartTime);
    
    // 遍历所有轨道，找出最早开始时间和最晚结束时间
    for (auto track = firstTrack_; track; track = track->next) {
        if (track->codecParms.trackType == ATTACHMENT_TYPE) {
            continue;
        }
        int64_t trackDuration = GetTrackDuration(track);
        mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_START_TIME>(trackStartTime);
        mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(trackTimeScale);

        double trackStartTimeSeconds = static_cast<double>(trackStartTime) / S_TO_US;
        double trackDurationInSeconds = static_cast<double>(trackDuration) / trackTimeScale;
        // 更新最早开始时间和最晚结束时间（以秒为单位）
        if (trackStartTimeSeconds < earliestStartTime) {
            earliestStartTime = trackStartTimeSeconds;
        }
        
        trackEndTime = trackStartTimeSeconds + trackDurationInSeconds;
        if (trackEndTime > latestEndTime) {
            latestEndTime = trackEndTime;
        }
    }
    int64_t fileDuration = static_cast<int64_t>(std::round(latestEndTime * S_TO_US));
    return fileDuration;
}

int64_t MPEG4AtomParser::FindMaxTrackDuration(int64_t& maxDuration, uint32_t& maxDurationTrackIndex)
{
    FileType fileType = FileType::MP4;
    mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType);
    bool isRawFile = (fileType == FileType::M4A); // M4V 和 M4A文件通常标记为M4A类型
    for (auto track = firstTrack_; track; track = track->next) {
        if (track->codecParms.trackType == ATTACHMENT_TYPE) {
            continue;
        }
        double trackDurationRaw = CalculateDuration(track, isRawFile);
        MEDIA_LOG_D("Calculated duration for track " PUBLIC_LOG_U32 ": " PUBLIC_LOG_F " s",
            track->trackIndex, trackDurationRaw);
        if (trackDurationRaw <= 0) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has no valid sample data", track->trackIndex);
            continue;
        }

        int64_t trackDuration = static_cast<int64_t>(trackDurationRaw);
        if (trackDuration > maxDuration) {
            maxDuration = trackDuration;
            maxDurationTrackIndex = track->trackIndex;
        }
    }
    
    return (maxDuration > 0) ? maxDuration : 0;
}

double MPEG4AtomParser::CalculateDuration(std::shared_ptr<Track> track, bool isRawFile)
{
    FALSE_RETURN_V_MSG_E(track != nullptr, 0.0, "Track is null");
    FALSE_RETURN_V_MSG_E(track->sampleHelper != nullptr, 0.0, "SampleHelper is null");
    FALSE_RETURN_V_MSG_E(!track->sampleHelper->sampleIndexEntry_.empty(), 0.0, "Sample data is empty");
    
    int32_t timeScale = 0;
    if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale) || timeScale <= 0) {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has invalid time scale", track->trackIndex);
        return 0.0;
    }
    
    int64_t firstPts = track->sampleHelper->sampleIndexEntry_.front().pts;
    int64_t baseDuration = 0;
    
    if (hasSidxBox_ && track->sidxDuration > 0) {
        // FMP4文件有sidx box，使用sidx中的duration
        baseDuration = track->sidxDuration;
    } else {
        // FMP4文件没有sidx box，使用trun duration
        baseDuration = track->sampleHelper->totalTrunDuration_;
    }
    
    // 根据文件类型计算最终duration
    int64_t totalDuration = baseDuration + track->elstInitEmptyEdit;
    int64_t durationUs = 0;
    if (__builtin_mul_overflow(totalDuration, S_TO_US, &durationUs)) {
        MEDIA_LOG_E("Mul overflow: [" PUBLIC_LOG_D64 "," PUBLIC_LOG_D64 "]", totalDuration, S_TO_US);
    }
    double duration = static_cast<double>(durationUs) / timeScale;
    duration = std::round(duration); // 四舍五入到整数微秒
    if (!isRawFile) {
        duration += static_cast<double>(firstPts) * S_TO_US / timeScale;
    }
    MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " duration: " PUBLIC_LOG_F, track->trackIndex, duration);
    return duration;
}

void MPEG4AtomParser::SetStartTime()
{
    int64_t minStartTime = INT64_MAX;  // 改为64位
    int64_t trackStartTime = 0;
    bool hasValidStartTime = false;
    for (auto track = firstTrack_; track; track = track->next) {
        if (track->codecParms.trackType == ATTACHMENT_TYPE) {
            continue;
        }
        if (mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_START_TIME>(trackStartTime)) {
            MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " already has start time", track->trackIndex);
        } else if (track->sampleHelper && track->sampleHelper->sampleIndexEntry_.size() > 0 &&
                   track->elstInitEmptyEdit == 0) {
                int64_t firstPts = track->sampleHelper->sampleIndexEntry_.front().pts;
                int32_t timeScale = 0;
                if (mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale) && timeScale > 0) {
                    FALSE_RETURN_MSG(
                        ConvertTimeScale(firstPts, S_TO_US, timeScale, &trackStartTime) == Status::OK, "Mul overflow");
                }
                mediaInfo_.tracks[track->trackIndex].Set<Tag::MEDIA_START_TIME>(trackStartTime);
        }
        if (trackStartTime < minStartTime) {
            minStartTime = trackStartTime;
            hasValidStartTime = true;
        }
    }
    int64_t containerStartTime = hasValidStartTime ? minStartTime : 0;
    mediaInfo_.general.Set<Tag::MEDIA_CONTAINER_START_TIME>(containerStartTime);
}

void MPEG4AtomParser::SetHEVCDefaultValue()
{
    ColorPrimary colorPrimaries = ColorPrimary::UNSPECIFIED;
    TransferCharacteristic colorTrc = TransferCharacteristic::UNSPECIFIED;
    MatrixCoefficient colorMatrix = MatrixCoefficient::UNSPECIFIED;
    bool colorRange = false;
    std::string mime;
    for (auto track = firstTrack_; track; track = track->next) {
        mediaInfo_.tracks[track->trackIndex].Get<Tag::MIME_TYPE>(mime);
        if (mime == MimeType::VIDEO_HEVC) {
            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries)) {
                ColorPrimary newColorPrimaries = ColorPrimary::UNSPECIFIED;
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_RANGE>(colorRange);
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_PRIMARIES>(newColorPrimaries);
            }

            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_COLOR_TRC>(colorTrc)) {
                TransferCharacteristic newColorTrc = TransferCharacteristic::UNSPECIFIED;
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_TRC>(newColorTrc);
            }

            if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix)) {
                MatrixCoefficient newColorMatrix = MatrixCoefficient::UNSPECIFIED;
                mediaInfo_.tracks[track->trackIndex].Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(newColorMatrix);
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
    return "und";
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS