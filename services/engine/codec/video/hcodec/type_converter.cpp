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

#include "type_converter.h"
#include "hcodec_log.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V3_0;

struct Protocol {
    OMX_VIDEO_CODINGTYPE omxCodingType;
    OHOS::HDI::Codec::V3_0::AvCodecRole hdiRole;
    string mime;
};
vector<Protocol> g_protocolTable = {
    {
        OMX_VIDEO_CodingAVC,
        OHOS::HDI::Codec::V3_0::AvCodecRole::MEDIA_ROLETYPE_VIDEO_AVC,
        string(CodecMimeType::VIDEO_AVC),
    },
    {
        static_cast<OMX_VIDEO_CODINGTYPE>(CODEC_OMX_VIDEO_CodingHEVC),
        OHOS::HDI::Codec::V3_0::AvCodecRole::MEDIA_ROLETYPE_VIDEO_HEVC,
        string(CodecMimeType::VIDEO_HEVC),
    },
};

vector<PixelFmt> g_pixelFmtTable = {
    {GRAPHIC_PIXEL_FMT_YCBCR_420_P,     VideoPixelFormat::YUVI420,  "I420"},
    {GRAPHIC_PIXEL_FMT_YCBCR_420_SP,    VideoPixelFormat::NV12,     "NV12"},
    {GRAPHIC_PIXEL_FMT_YCRCB_420_SP,    VideoPixelFormat::NV21,     "NV21"},
    {GRAPHIC_PIXEL_FMT_RGBA_8888,       VideoPixelFormat::RGBA,     "RGBA"},
    {GRAPHIC_PIXEL_FMT_YCBCR_P010,      VideoPixelFormat::NV12,     "NV12_10bit"},
    {GRAPHIC_PIXEL_FMT_YCRCB_P010,      VideoPixelFormat::NV21,     "NV21_10bit"},
};

struct AVCProfileMapping {
    OMX_VIDEO_AVCPROFILETYPE omxProfile;
    AVCProfile innerProfile;
};
vector<AVCProfileMapping> g_avcProfileTable = {
    { OMX_VIDEO_AVCProfileBaseline, AVC_PROFILE_BASELINE },
    { OMX_VIDEO_AVCProfileMain,     AVC_PROFILE_MAIN },
    { OMX_VIDEO_AVCProfileExtended, AVC_PROFILE_EXTENDED },
    { OMX_VIDEO_AVCProfileHigh,     AVC_PROFILE_HIGH },
    { OMX_VIDEO_AVCProfileHigh10,   AVC_PROFILE_HIGH_10 },
    { OMX_VIDEO_AVCProfileHigh422,  AVC_PROFILE_HIGH_422 },
    { OMX_VIDEO_AVCProfileHigh444,  AVC_PROFILE_HIGH_444 },
};

struct AVCLevelMapping {
    OMX_VIDEO_AVCLEVELTYPE omxLevel;
    AVCLevel innerLevel;
};
vector<AVCLevelMapping> g_avcLevelTable = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL_1 },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL_1b },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL_11 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL_12 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL_13 },
    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL_2 },
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL_21 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL_22 },
    { OMX_VIDEO_AVCLevel3,  AVC_LEVEL_3 },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL_31 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL_32 },
    { OMX_VIDEO_AVCLevel4,  AVC_LEVEL_4 },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL_41 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL_42 },
    { OMX_VIDEO_AVCLevel5,  AVC_LEVEL_5 },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL_51 },
};

struct HEVCProfileMapping {
    CodecHevcProfile omxProfile;
    HEVCProfile innerProfile;
};
vector<HEVCProfileMapping> g_hevcProfileTable = {
    { CODEC_HEVC_PROFILE_MAIN,              HEVC_PROFILE_MAIN },
    { CODEC_HEVC_PROFILE_MAIN10,            HEVC_PROFILE_MAIN_10 },
    { CODEC_HEVC_PROFILE_MAIN_STILL,        HEVC_PROFILE_MAIN_STILL },
    { CODEC_HEVC_PROFILE_MAIN10_HDR10,      HEVC_PROFILE_MAIN_10_HDR10 },
    { CODEC_HEVC_PROFILE_MAIN10_HDR10_PLUS, HEVC_PROFILE_MAIN_10_HDR10_PLUS },
};

struct HEVCLevelMapping {
    CodecHevcLevel omxLevel;
    HEVCLevel innerLevel;
};
vector<HEVCLevelMapping> g_hevcLevelTable = {
    { CODEC_HEVC_MAIN_TIER_LEVEL1,  HEVC_LEVEL_1 },
    { CODEC_HEVC_HIGH_TIER_LEVEL1,  HEVC_LEVEL_1 },
    { CODEC_HEVC_MAIN_TIER_LEVEL2,  HEVC_LEVEL_2 },
    { CODEC_HEVC_HIGH_TIER_LEVEL2,  HEVC_LEVEL_2 },
    { CODEC_HEVC_MAIN_TIER_LEVEL21, HEVC_LEVEL_21 },
    { CODEC_HEVC_HIGH_TIER_LEVEL21, HEVC_LEVEL_21 },
    { CODEC_HEVC_MAIN_TIER_LEVEL3,  HEVC_LEVEL_3 },
    { CODEC_HEVC_HIGH_TIER_LEVEL3,  HEVC_LEVEL_3 },
    { CODEC_HEVC_MAIN_TIER_LEVEL31, HEVC_LEVEL_31 },
    { CODEC_HEVC_HIGH_TIER_LEVEL31, HEVC_LEVEL_31 },
    { CODEC_HEVC_MAIN_TIER_LEVEL4,  HEVC_LEVEL_4 },
    { CODEC_HEVC_HIGH_TIER_LEVEL4,  HEVC_LEVEL_4 },
    { CODEC_HEVC_MAIN_TIER_LEVEL41, HEVC_LEVEL_41 },
    { CODEC_HEVC_HIGH_TIER_LEVEL41, HEVC_LEVEL_41 },
    { CODEC_HEVC_MAIN_TIER_LEVEL5,  HEVC_LEVEL_5 },
    { CODEC_HEVC_HIGH_TIER_LEVEL5,  HEVC_LEVEL_5 },
    { CODEC_HEVC_MAIN_TIER_LEVEL51, HEVC_LEVEL_51 },
    { CODEC_HEVC_HIGH_TIER_LEVEL51, HEVC_LEVEL_51 },
    { CODEC_HEVC_MAIN_TIER_LEVEL52, HEVC_LEVEL_52 },
    { CODEC_HEVC_HIGH_TIER_LEVEL52, HEVC_LEVEL_52 },
    { CODEC_HEVC_MAIN_TIER_LEVEL6,  HEVC_LEVEL_6 },
    { CODEC_HEVC_HIGH_TIER_LEVEL6,  HEVC_LEVEL_6 },
    { CODEC_HEVC_MAIN_TIER_LEVEL61, HEVC_LEVEL_61 },
    { CODEC_HEVC_HIGH_TIER_LEVEL61, HEVC_LEVEL_61 },
    { CODEC_HEVC_MAIN_TIER_LEVEL62, HEVC_LEVEL_62 },
    { CODEC_HEVC_HIGH_TIER_LEVEL62, HEVC_LEVEL_62 },
};

optional<AVCodecType> TypeConverter::HdiCodecTypeToInnerCodecType(OHOS::HDI::Codec::V3_0::CodecType type)
{
    static const map<CodecType, AVCodecType> table = {
        {VIDEO_DECODER, AVCODEC_TYPE_VIDEO_DECODER},
        {VIDEO_ENCODER, AVCODEC_TYPE_VIDEO_ENCODER}
    };
    auto it = table.find(type);
    if (it == table.end()) {
        LOGW("unknown codecType %d", type);
        return std::nullopt;
    }
    return it->second;
}

std::optional<OMX_VIDEO_CODINGTYPE> TypeConverter::HdiRoleToOmxCodingType(AvCodecRole role)
{
    auto it = find_if(g_protocolTable.begin(), g_protocolTable.end(), [role](const Protocol& p) {
        return p.hdiRole == role;
    });
    if (it != g_protocolTable.end()) {
        return it->omxCodingType;
    }
    LOGW("unknown AvCodecRole %d", role);
    return nullopt;
}

string TypeConverter::HdiRoleToMime(AvCodecRole role)
{
    auto it = find_if(g_protocolTable.begin(), g_protocolTable.end(), [role](const Protocol& p) {
        return p.hdiRole == role;
    });
    if (it != g_protocolTable.end()) {
        return it->mime;
    }
    LOGW("unknown AvCodecRole %d", role);
    return {};
}

std::optional<PixelFmt> TypeConverter::GraphicFmtToFmt(GraphicPixelFormat format)
{
    auto it = find_if(g_pixelFmtTable.begin(), g_pixelFmtTable.end(), [format](const PixelFmt& p) {
        return p.graphicFmt == format;
    });
    if (it != g_pixelFmtTable.end()) {
        return *it;
    }
    LOGW("unknown GraphicPixelFormat %d", format);
    return nullopt;
}

std::optional<PixelFmt> TypeConverter::InnerFmtToFmt(VideoPixelFormat format)
{
    auto it = find_if(g_pixelFmtTable.begin(), g_pixelFmtTable.end(), [format](const PixelFmt& p) {
        return p.innerFmt == format;
    });
    if (it != g_pixelFmtTable.end()) {
        return *it;
    }
    LOGW("unknown VideoPixelFormat %d", format);
    return nullopt;
}

std::optional<GraphicPixelFormat> TypeConverter::InnerFmtToDisplayFmt(VideoPixelFormat format)
{
    auto it = find_if(g_pixelFmtTable.begin(), g_pixelFmtTable.end(), [format](const PixelFmt& p) {
        return p.innerFmt == format;
    });
    if (it != g_pixelFmtTable.end()) {
        return it->graphicFmt;
    }
    LOGW("unknown VideoPixelFormat %d", format);
    return nullopt;
}

std::optional<VideoPixelFormat> TypeConverter::DisplayFmtToInnerFmt(GraphicPixelFormat format)
{
    auto it = find_if(g_pixelFmtTable.begin(), g_pixelFmtTable.end(), [format](const PixelFmt& p) {
        return p.graphicFmt == format;
    });
    if (it != g_pixelFmtTable.end()) {
        return it->innerFmt;
    }
    LOGW("unknown GraphicPixelFormat %d", format);
    return nullopt;
}

std::optional<GraphicTransformType> TypeConverter::InnerRotateToDisplayRotate(VideoRotation rotate)
{
    static const map<VideoRotation, GraphicTransformType> table = {
        { VIDEO_ROTATION_0, GRAPHIC_ROTATE_NONE },
        { VIDEO_ROTATION_90, GRAPHIC_ROTATE_270 },
        { VIDEO_ROTATION_180, GRAPHIC_ROTATE_180 },
        { VIDEO_ROTATION_270, GRAPHIC_ROTATE_90 },
    };
    auto it = table.find(rotate);
    if (it == table.end()) {
        LOGW("unknown VideoRotation %u", rotate);
        return std::nullopt;
    }
    return it->second;
}

Primaries TypeConverter::InnerPrimaryToOmxPrimary(ColorPrimary primary)
{
    static const map<ColorPrimary, Primaries> table = {
        { COLOR_PRIMARY_BT709,        PRIMARIES_BT709 },
        { COLOR_PRIMARY_UNSPECIFIED,  PRIMARIES_UNSPECIFIED },
        { COLOR_PRIMARY_BT470_M,      PRIMARIES_BT470_6M },
        { COLOR_PRIMARY_BT601_625,    PRIMARIES_BT601_625 },
        { COLOR_PRIMARY_BT601_525,    PRIMARIES_BT601_525 },
        { COLOR_PRIMARY_SMPTE_ST240,  PRIMARIES_BT601_525 },
        { COLOR_PRIMARY_GENERIC_FILM, PRIMARIES_GENERICFILM },
        { COLOR_PRIMARY_BT2020,       PRIMARIES_BT2020 },
        { COLOR_PRIMARY_SMPTE_ST428,  PRIMARIES_MAX },
        { COLOR_PRIMARY_P3DCI,        PRIMARIES_MAX },
        { COLOR_PRIMARY_P3D65,        PRIMARIES_MAX },
    };
    auto it = table.find(primary);
    if (it == table.end()) {
        LOGW("unknown ColorPrimary %d, use unspecified instead", primary);
        return PRIMARIES_UNSPECIFIED;
    }
    return it->second;
}

Transfer TypeConverter::InnerTransferToOmxTransfer(TransferCharacteristic transfer)
{
    static const map<TransferCharacteristic, Transfer> table = {
        { TRANSFER_CHARACTERISTIC_BT709,           TRANSFER_SMPTE170 },
        { TRANSFER_CHARACTERISTIC_UNSPECIFIED,     TRANSFER_UNSPECIFIED },
        { TRANSFER_CHARACTERISTIC_GAMMA_2_2,       TRANSFER_GAMMA22 },
        { TRANSFER_CHARACTERISTIC_GAMMA_2_8,       TRANSFER_GAMMA28 },
        { TRANSFER_CHARACTERISTIC_BT601,           TRANSFER_SMPTE170 },
        { TRANSFER_CHARACTERISTIC_SMPTE_ST240,     TRANSFER_SMPTE240 },
        { TRANSFER_CHARACTERISTIC_LINEAR,          TRANSFER_LINEAR },
        { TRANSFER_CHARACTERISTIC_LOG,             TRANSFER_MAX },
        { TRANSFER_CHARACTERISTIC_LOG_SQRT,        TRANSFER_MAX },
        { TRANSFER_CHARACTERISTIC_IEC_61966_2_4,   TRANSFER_XVYCC },
        { TRANSFER_CHARACTERISTIC_BT1361,          TRANSFER_BT1361 },
        { TRANSFER_CHARACTERISTIC_IEC_61966_2_1,   TRANSFER_MAX },
        { TRANSFER_CHARACTERISTIC_BT2020_10BIT,    TRANSFER_SMPTE170 },
        { TRANSFER_CHARACTERISTIC_BT2020_12BIT,    TRANSFER_SMPTE170 },
        { TRANSFER_CHARACTERISTIC_PQ,              TRANSFER_PQ },
        { TRANSFER_CHARACTERISTIC_SMPTE_ST428,     TRANSFER_ST428 },
        { TRANSFER_CHARACTERISTIC_HLG,             TRANSFER_HLG },
    };
    auto it = table.find(transfer);
    if (it == table.end()) {
        LOGW("unknown TransferCharacteristic %d, use unspecified instead", transfer);
        return TRANSFER_UNSPECIFIED;
    }
    return it->second;
}

MatrixCoeffs TypeConverter::InnerMatrixToOmxMatrix(MatrixCoefficient matrix)
{
    static const map<MatrixCoefficient, MatrixCoeffs> table = {
        { MATRIX_COEFFICIENT_IDENTITY,          MATRIX_MAX },
        { MATRIX_COEFFICIENT_BT709,             MATRIX_BT709 },
        { MATRIX_COEFFICIENT_UNSPECIFIED,       MATRIX_UNSPECIFED },
        { MATRIX_COEFFICIENT_FCC,               MATRIX_FCC },
        { MATRIX_COEFFICIENT_BT601_625,         MATRIX_BT601 },
        { MATRIX_COEFFICIENT_BT601_525,         MATRIX_BT601 },
        { MATRIX_COEFFICIENT_SMPTE_ST240,       MATRIX_SMPTE240 },
        { MATRIX_COEFFICIENT_YCGCO,             MATRIX_MAX },
        { MATRIX_COEFFICIENT_BT2020_NCL,        MATRIX_BT2020 },
        { MATRIX_COEFFICIENT_BT2020_CL,         MATRIX_BT2020CONSTANT },
        { MATRIX_COEFFICIENT_SMPTE_ST2085,      MATRIX_MAX },
        { MATRIX_COEFFICIENT_CHROMATICITY_NCL,  MATRIX_BT2020 },
        { MATRIX_COEFFICIENT_CHROMATICITY_CL,   MATRIX_BT2020CONSTANT },
        { MATRIX_COEFFICIENT_ICTCP,             MATRIX_MAX },
    };
    auto it = table.find(matrix);
    if (it == table.end()) {
        LOGW("unknown MatrixCoefficient %d, use unspecified instead", matrix);
        return MATRIX_UNSPECIFED;
    }
    return it->second;
}

std::optional<AVCProfile> TypeConverter::OmxAvcProfileToInnerProfile(OMX_VIDEO_AVCPROFILETYPE profile)
{
    auto it = find_if(g_avcProfileTable.begin(), g_avcProfileTable.end(), [profile](const AVCProfileMapping& p) {
        return p.omxProfile == profile;
    });
    if (it != g_avcProfileTable.end()) {
        return it->innerProfile;
    }
    LOGW("unknown OMX_VIDEO_AVCPROFILETYPE %d", profile);
    return nullopt;
}

std::optional<AVCLevel> TypeConverter::OmxAvcLevelToInnerLevel(OMX_VIDEO_AVCLEVELTYPE level)
{
    auto it = find_if(g_avcLevelTable.begin(), g_avcLevelTable.end(), [level](const AVCLevelMapping& p) {
        return p.omxLevel == level;
    });
    if (it != g_avcLevelTable.end()) {
        return it->innerLevel;
    }
    LOGW("unknown OMX_VIDEO_AVCLEVELTYPE %d", level);
    return nullopt;
}

std::optional<HEVCProfile> TypeConverter::OmxHevcProfileToInnerProfile(CodecHevcProfile profile)
{
    auto it = find_if(g_hevcProfileTable.begin(), g_hevcProfileTable.end(), [profile](const HEVCProfileMapping& p) {
        return p.omxProfile == profile;
    });
    if (it != g_hevcProfileTable.end()) {
        return it->innerProfile;
    }
    LOGW("unknown CodecHevcProfile %d", profile);
    return nullopt;
}

std::optional<HEVCLevel> TypeConverter::OmxHevcLevelToInnerLevel(CodecHevcLevel level)
{
    auto it = find_if(g_hevcLevelTable.begin(), g_hevcLevelTable.end(), [level](const HEVCLevelMapping& p) {
        return p.omxLevel == level;
    });
    if (it != g_hevcLevelTable.end()) {
        return it->innerLevel;
    }
    LOGW("unknown CodecHevcLevel %d", level);
    return nullopt;
}

std::optional<OMX_VIDEO_AVCPROFILETYPE> TypeConverter::InnerAvcProfileToOmxProfile(AVCProfile profile)
{
    auto it = find_if(g_avcProfileTable.begin(), g_avcProfileTable.end(), [profile](const AVCProfileMapping& p) {
        return p.innerProfile == profile;
    });
    if (it != g_avcProfileTable.end()) {
        return it->omxProfile;
    }
    LOGW("unknown AVCProfile %d", profile);
    return nullopt;
}

std::optional<CodecHevcProfile> TypeConverter::InnerHevcProfileToOmxProfile(HEVCProfile profile)
{
    auto it = find_if(g_hevcProfileTable.begin(), g_hevcProfileTable.end(), [profile](const HEVCProfileMapping& p) {
        return p.innerProfile == profile;
    });
    if (it != g_hevcProfileTable.end()) {
        return it->omxProfile;
    }
    LOGW("unknown CodecHevcProfile %d", profile);
    return nullopt;
}

std::optional<VideoEncodeBitrateMode> TypeConverter::HdiBitrateModeToInnerMode(BitRateMode mode)
{
    static const map<BitRateMode, VideoEncodeBitrateMode> table = {
        {BIT_RATE_MODE_VBR, VBR},
        {BIT_RATE_MODE_CBR, CBR},
        {BIT_RATE_MODE_CQ,  CQ},
    };
    auto it = table.find(mode);
    if (it == table.end()) {
        LOGW("unknown BitRateMode %d", mode);
        return std::nullopt;
    }
    return it->second;
}
}