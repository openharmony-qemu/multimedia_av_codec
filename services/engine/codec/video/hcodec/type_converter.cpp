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
using namespace CodecHDI;

struct Protocol {
    OMX_VIDEO_CODINGTYPE omxCodingType;
    CodecHDI::AvCodecRole hdiRole;
    string mime;
};
vector<Protocol> g_protocolTable = {
    {
        OMX_VIDEO_CodingAVC,
        CodecHDI::AvCodecRole::MEDIA_ROLETYPE_VIDEO_AVC,
        string(CodecMimeType::VIDEO_AVC),
    },
    {
        static_cast<OMX_VIDEO_CODINGTYPE>(CODEC_OMX_VIDEO_CodingHEVC),
        CodecHDI::AvCodecRole::MEDIA_ROLETYPE_VIDEO_HEVC,
        string(CodecMimeType::VIDEO_HEVC),
    },
    {
        static_cast<OMX_VIDEO_CODINGTYPE>(CODEC_OMX_VIDEO_CodingVVC),
        OHOS::HDI::Codec::V4_0::AvCodecRole::MEDIA_ROLETYPE_VIDEO_VVC,
        string("video/vvc"),
    },
};

vector<PixelFmt> g_pixelFmtTable = {
    {GRAPHIC_PIXEL_FMT_YCBCR_420_P,     VideoPixelFormat::YUVI420,  "I420"},
    {GRAPHIC_PIXEL_FMT_YCBCR_420_SP,    VideoPixelFormat::NV12,     "NV12"},
    {GRAPHIC_PIXEL_FMT_YCRCB_420_SP,    VideoPixelFormat::NV21,     "NV21"},
    {GRAPHIC_PIXEL_FMT_RGBA_8888,       VideoPixelFormat::RGBA,     "RGBA"},
    {GRAPHIC_PIXEL_FMT_YCBCR_P010,      VideoPixelFormat::NV12,     "NV12_10bit"},
    {GRAPHIC_PIXEL_FMT_YCRCB_P010,      VideoPixelFormat::NV21,     "NV21_10bit"},
    {GRAPHIC_PIXEL_FMT_RGBA_1010102,    VideoPixelFormat::RGBA1010102,     "RGBA1010102"},
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
    { static_cast<OMX_VIDEO_AVCLEVELTYPE>(OMX_VIDEO_AVC_LEVEL52), AVC_LEVEL_52 },
    { static_cast<OMX_VIDEO_AVCLEVELTYPE>(OMX_VIDEO_AVC_LEVEL6), AVC_LEVEL_6 },
    { static_cast<OMX_VIDEO_AVCLEVELTYPE>(OMX_VIDEO_AVC_LEVEL61), AVC_LEVEL_61 },
    { static_cast<OMX_VIDEO_AVCLEVELTYPE>(OMX_VIDEO_AVC_LEVEL62), AVC_LEVEL_62 },
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

struct VVCProfileMapping {
    CodecVvcProfile omxProfile;
    VVCProfile innerProfile;
};
vector<VVCProfileMapping> g_vvcProfileTable = {
    { CODEC_VVC_PROFILE_MAIN10,              VVC_PROFILE_MAIN_10 },
    { CODEC_VVC_PROFILE_MAIN10_STILL,        VVC_PROFILE_MAIN_10_STILL },
    { CODEC_VVC_PROFILE_MAIN10_444,          VVC_PROFILE_MAIN_10_444 },
    { CODEC_VVC_PROFILE_MAIN10_444_STILL,    VVC_PROFILE_MAIN_10_444_STILL },
    { CODEC_VVC_PROFILE_MULTI_MAIN10,        VVC_PROFILE_MULTI_MAIN_10 },
    { CODEC_VVC_PROFILE_MULTI_MAIN10_444,    VVC_PROFILE_MULTI_MAIN_10_444 },
    { CODEC_VVC_PROFILE_MAIN12,              VVC_PROFILE_MAIN_12 },
    { CODEC_VVC_PROFILE_MAIN12_INTRA,        VVC_PROFILE_MAIN_12_INTRA },
    { CODEC_VVC_PROFILE_MAIN12_STILL,        VVC_PROFILE_MAIN_12_STILL },
    { CODEC_VVC_PROFILE_MAIN12_444,          VVC_PROFILE_MAIN_12_444 },
    { CODEC_VVC_PROFILE_MAIN12_444_INTRA,    VVC_PROFILE_MAIN_12_444_INTRA },
    { CODEC_VVC_PROFILE_MAIN12_444_STILL,    VVC_PROFILE_MAIN_12_444_STILL },
    { CODEC_VVC_PROFILE_MAIN16_444,          VVC_PROFILE_MAIN_16_444 },
    { CODEC_VVC_PROFILE_MAIN16_444_INTRA,    VVC_PROFILE_MAIN_16_444_INTRA },
    { CODEC_VVC_PROFILE_MAIN16_444_STILL,    VVC_PROFILE_MAIN_16_444_STILL },
};

struct VVCLevelMapping {
    CodecVvcLevel omxLevel;
    VVCLevel innerLevel;
};
vector<VVCLevelMapping> g_vvcLevelTable = {
    { CODEC_VVC_MAIN_TIER_LEVEL1,   VVC_LEVEL_1 },
    { CODEC_VVC_HIGH_TIER_LEVEL1,   VVC_LEVEL_1 },
    { CODEC_VVC_MAIN_TIER_LEVEL2,   VVC_LEVEL_2 },
    { CODEC_VVC_HIGH_TIER_LEVEL2,   VVC_LEVEL_2 },
    { CODEC_VVC_MAIN_TIER_LEVEL21,  VVC_LEVEL_21 },
    { CODEC_VVC_HIGH_TIER_LEVEL21,  VVC_LEVEL_21 },
    { CODEC_VVC_MAIN_TIER_LEVEL3,   VVC_LEVEL_3 },
    { CODEC_VVC_HIGH_TIER_LEVEL3,   VVC_LEVEL_3 },
    { CODEC_VVC_MAIN_TIER_LEVEL31,  VVC_LEVEL_31 },
    { CODEC_VVC_HIGH_TIER_LEVEL31,  VVC_LEVEL_31 },
    { CODEC_VVC_MAIN_TIER_LEVEL4,   VVC_LEVEL_4 },
    { CODEC_VVC_HIGH_TIER_LEVEL4,   VVC_LEVEL_4 },
    { CODEC_VVC_MAIN_TIER_LEVEL41,  VVC_LEVEL_41 },
    { CODEC_VVC_HIGH_TIER_LEVEL41,  VVC_LEVEL_41 },
    { CODEC_VVC_MAIN_TIER_LEVEL5,   VVC_LEVEL_5 },
    { CODEC_VVC_HIGH_TIER_LEVEL5,   VVC_LEVEL_5 },
    { CODEC_VVC_MAIN_TIER_LEVEL51,  VVC_LEVEL_51 },
    { CODEC_VVC_HIGH_TIER_LEVEL51,  VVC_LEVEL_51 },
    { CODEC_VVC_MAIN_TIER_LEVEL52,  VVC_LEVEL_52 },
    { CODEC_VVC_HIGH_TIER_LEVEL52,  VVC_LEVEL_52 },
    { CODEC_VVC_MAIN_TIER_LEVEL6,   VVC_LEVEL_6 },
    { CODEC_VVC_HIGH_TIER_LEVEL6,   VVC_LEVEL_6 },
    { CODEC_VVC_MAIN_TIER_LEVEL61,  VVC_LEVEL_61 },
    { CODEC_VVC_HIGH_TIER_LEVEL61,  VVC_LEVEL_61 },
    { CODEC_VVC_MAIN_TIER_LEVEL62,  VVC_LEVEL_62 },
    { CODEC_VVC_HIGH_TIER_LEVEL62,  VVC_LEVEL_62 },
    { CODEC_VVC_MAIN_TIER_LEVEL63,  VVC_LEVEL_63 },
    { CODEC_VVC_HIGH_TIER_LEVEL63,  VVC_LEVEL_63 },
    { CODEC_VVC_MAIN_TIER_LEVEL155, VVC_LEVEL_155 },
    { CODEC_VVC_HIGH_TIER_LEVEL155, VVC_LEVEL_155 },
};

struct VVCMaxLevelMapping {
    VVCLevel maxLevel;
    vector<int32_t> allLevels;
};

vector<VVCMaxLevelMapping> g_vvcMaxLevelTable = {
    { VVC_LEVEL_1,    {VVC_LEVEL_1} },
    { VVC_LEVEL_2,    {VVC_LEVEL_1, VVC_LEVEL_2} },
    { VVC_LEVEL_21,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21} },
    { VVC_LEVEL_3,    {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3} },
    { VVC_LEVEL_31,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31} },
    { VVC_LEVEL_4,    {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4} },
    { VVC_LEVEL_41,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41} },
    { VVC_LEVEL_5,    {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5} },
    { VVC_LEVEL_51,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51} },
    { VVC_LEVEL_52,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51, VVC_LEVEL_52} },
    { VVC_LEVEL_6,    {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51, VVC_LEVEL_52, VVC_LEVEL_6} },
    { VVC_LEVEL_61,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51, VVC_LEVEL_52, VVC_LEVEL_6, VVC_LEVEL_61} },
    { VVC_LEVEL_62,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51, VVC_LEVEL_52, VVC_LEVEL_6, VVC_LEVEL_61, VVC_LEVEL_62} },
    { VVC_LEVEL_63,   {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51, VVC_LEVEL_52, VVC_LEVEL_6, VVC_LEVEL_61, VVC_LEVEL_62,
                       VVC_LEVEL_63} },
    { VVC_LEVEL_155,  {VVC_LEVEL_1, VVC_LEVEL_2, VVC_LEVEL_21, VVC_LEVEL_3, VVC_LEVEL_31, VVC_LEVEL_4, VVC_LEVEL_41,
                       VVC_LEVEL_5, VVC_LEVEL_51, VVC_LEVEL_52, VVC_LEVEL_6, VVC_LEVEL_61, VVC_LEVEL_62,
                       VVC_LEVEL_63, VVC_LEVEL_155} },
};

struct BitrateModeMapping {
    VideoEncodeBitrateMode innerMode;
    OMX_VIDEO_CONTROLRATETYPE omxMode;
};
vector<BitrateModeMapping> g_bitrateModeTable {
    {CBR, OMX_Video_ControlRateConstant},
    {VBR, OMX_Video_ControlRateVariable},
    {CBR_VIDEOCALL, static_cast<OMX_VIDEO_CONTROLRATETYPE>(OMX_Video_ControlRateConstantWithRlambda)},
    {CQ, static_cast<OMX_VIDEO_CONTROLRATETYPE>(OMX_Video_ControlRateConstantWithCQ)},
    {SQR, static_cast<OMX_VIDEO_CONTROLRATETYPE>(OMX_Video_ControlRateConstantWithSQR)},
    {CRF, static_cast<OMX_VIDEO_CONTROLRATETYPE>(OMX_Video_ControlRateConstantWithCRF)},
};

optional<AVCodecType> TypeConverter::HdiCodecTypeToInnerCodecType(CodecHDI::CodecType type)
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

std::optional<VVCProfile> TypeConverter::OmxVvcProfileToInnerProfile(CodecVvcProfile profile)
{
    auto it = find_if(g_vvcProfileTable.begin(), g_vvcProfileTable.end(), [profile](const VVCProfileMapping& p) {
        return p.omxProfile == profile;
    });
    if (it != g_vvcProfileTable.end()) {
        return it->innerProfile;
    }
    LOGW("unknown CodecVvcProfile %d", profile);
    return nullopt;
}

std::optional<VVCLevel> TypeConverter::OmxVvcLevelToInnerLevel(CodecVvcLevel level)
{
    auto it = find_if(g_vvcLevelTable.begin(), g_vvcLevelTable.end(), [level](const VVCLevelMapping& p) {
        return p.omxLevel == level;
    });
    if (it != g_vvcLevelTable.end()) {
        return it->innerLevel;
    }
    LOGW("unknown CodecVvcLevel %d", level);
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

std::optional<CodecVvcProfile> TypeConverter::InnerVvcProfileToOmxProfile(VVCProfile profile)
{
    auto it = find_if(g_vvcProfileTable.begin(), g_vvcProfileTable.end(), [profile](const VVCProfileMapping& p) {
        return p.innerProfile == profile;
    });
    if (it != g_vvcProfileTable.end()) {
        return it->omxProfile;
    }
    LOGW("unknown CodecVvcProfile %d", profile);
    return nullopt;
}

std::optional<std::vector<int32_t>> TypeConverter::InnerVvcMaxLevelToAllLevels(VVCLevel maxLevel)
{
    auto it = find_if(g_vvcMaxLevelTable.begin(), g_vvcMaxLevelTable.end(), [maxLevel](const VVCMaxLevelMapping& p) {
        return p.maxLevel == maxLevel;
    });
    if (it != g_vvcMaxLevelTable.end()) {
        return it->allLevels;
    }
    LOGW("unknown VvcMaxLevel %d", maxLevel);
    return nullopt;
}

std::optional<OMX_VIDEO_CONTROLRATETYPE> TypeConverter::InnerModeToOmxBitrateMode(VideoEncodeBitrateMode mode)
{
    auto it = std::find_if(g_bitrateModeTable.begin(), g_bitrateModeTable.end(), [mode](const BitrateModeMapping& p) {
        return p.innerMode == mode;
    });
    if (it == g_bitrateModeTable.end()) {
        LOGW("unknown BitRateMode %d", mode);
        return std::nullopt;
    }
    return it->omxMode;
}

std::optional<VideoEncodeBitrateMode> TypeConverter::OmxBitrateModeToInnerMode(OMX_VIDEO_CONTROLRATETYPE mode)
{
    auto it = std::find_if(g_bitrateModeTable.begin(), g_bitrateModeTable.end(), [mode](const BitrateModeMapping& p) {
        return p.omxMode == mode;
    });
    if (it == g_bitrateModeTable.end()) {
        LOGW("unknown BitRateMode %d", mode);
        return std::nullopt;
    }
    return it->innerMode;
}
}