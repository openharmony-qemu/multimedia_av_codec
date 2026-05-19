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

#define HST_LOG_TAG "Converter"

#include <vector>
#include <algorithm>
#include <iconv.h>
#include "avcodec_log.h"
#include "common/log.h"
#include "converter.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "Converter"};
}
namespace OHOS {
namespace Media {
namespace Plugins {
const unsigned char MASK_80 = 0x80;
const unsigned char MASK_C0 = 0xC0;
const int32_t MIN_BYTES = 2;
const int32_t MAX_BYTES = 6;
const uint32_t DOUBLE_BYTES = 2;

// ffmpeg channel layout to histreamer channel layout
const std::vector<std::pair<AudioChannelLayout, uint64_t>> g_toFFMPEGChannelLayout = {
    {AudioChannelLayout::MONO, AV_CH_LAYOUT_MONO},
    {AudioChannelLayout::STEREO, AV_CH_LAYOUT_STEREO},
    {AudioChannelLayout::CH_2POINT1, AV_CH_LAYOUT_2POINT1},
    {AudioChannelLayout::CH_2_1, AV_CH_LAYOUT_2_1},
    {AudioChannelLayout::SURROUND, AV_CH_LAYOUT_SURROUND},
    {AudioChannelLayout::CH_3POINT1, AV_CH_LAYOUT_3POINT1},
    {AudioChannelLayout::CH_4POINT0, AV_CH_LAYOUT_4POINT0},
    {AudioChannelLayout::CH_4POINT1, AV_CH_LAYOUT_4POINT1},
    {AudioChannelLayout::CH_2_2, AV_CH_LAYOUT_2_2},
    {AudioChannelLayout::QUAD, AV_CH_LAYOUT_QUAD},
    {AudioChannelLayout::CH_5POINT0, AV_CH_LAYOUT_5POINT0},
    {AudioChannelLayout::CH_5POINT1, AV_CH_LAYOUT_5POINT1},
    {AudioChannelLayout::CH_5POINT0_BACK, AV_CH_LAYOUT_5POINT0_BACK},
    {AudioChannelLayout::CH_5POINT1_BACK, AV_CH_LAYOUT_5POINT1_BACK},
    {AudioChannelLayout::CH_6POINT0, AV_CH_LAYOUT_6POINT0},
    {AudioChannelLayout::CH_6POINT0_FRONT, AV_CH_LAYOUT_6POINT0_FRONT},
    {AudioChannelLayout::HEXAGONAL, AV_CH_LAYOUT_HEXAGONAL},
    {AudioChannelLayout::CH_6POINT1, AV_CH_LAYOUT_6POINT1},
    {AudioChannelLayout::CH_6POINT1_BACK, AV_CH_LAYOUT_6POINT1_BACK},
    {AudioChannelLayout::CH_6POINT1_FRONT, AV_CH_LAYOUT_6POINT1_FRONT},
    {AudioChannelLayout::CH_7POINT0, AV_CH_LAYOUT_7POINT0},
    {AudioChannelLayout::CH_7POINT0_FRONT, AV_CH_LAYOUT_7POINT0_FRONT},
    {AudioChannelLayout::CH_7POINT1, AV_CH_LAYOUT_7POINT1},
    {AudioChannelLayout::CH_7POINT1_WIDE, AV_CH_LAYOUT_7POINT1_WIDE},
    {AudioChannelLayout::CH_7POINT1_WIDE_BACK, AV_CH_LAYOUT_7POINT1_WIDE_BACK},
    {AudioChannelLayout::OCTAGONAL, AV_CH_LAYOUT_OCTAGONAL},
    {AudioChannelLayout::HEXADECAGONAL, AV_CH_LAYOUT_HEXADECAGONAL},
    {AudioChannelLayout::STEREO_DOWNMIX, AV_CH_LAYOUT_STEREO_DOWNMIX},
};

const std::vector<std::pair<AudioChannelLayout, int>> g_audioVividChannelLayoutMap = {
    {AudioChannelLayout::MONO, 1},
    {AudioChannelLayout::STEREO, 2},
    {AudioChannelLayout::CH_4POINT0, 4},
    {AudioChannelLayout::CH_5POINT1, 6},
    {AudioChannelLayout::CH_7POINT1, 8},
    {AudioChannelLayout::CH_5POINT1POINT2, 8},
    {AudioChannelLayout::CH_5POINT1POINT4, 10},
    {AudioChannelLayout::CH_7POINT1POINT2, 10},
    {AudioChannelLayout::CH_7POINT1POINT4, 12},
    {AudioChannelLayout::HOA_ORDER1_ACN_SN3D, 4},
    {AudioChannelLayout::HOA_ORDER2_ACN_SN3D, 9},
    {AudioChannelLayout::HOA_ORDER3_ACN_SN3D, 16},
};

const std::vector<std::pair<int, AudioChannelLayout>> g_channelLayoutDefaultMap = {
    {2, AudioChannelLayout::STEREO},             // 2: STEREO
    {4, AudioChannelLayout::CH_4POINT0},         // 4: CH_4POINT0
    {6, AudioChannelLayout::CH_5POINT1},         // 6: CH_5POINT1
    {8, AudioChannelLayout::CH_5POINT1POINT2},   // 8: CH_5POINT1POINT2
    {9, AudioChannelLayout::HOA_ORDER2_ACN_N3D}, // 9: HOA_ORDER2_ACN_N3D
    {10, AudioChannelLayout::CH_7POINT1POINT2},  // 10: CH_7POINT1POINT2 or CH_5POINT1POINT4 ?
    {12, AudioChannelLayout::CH_7POINT1POINT4},  // 12: CH_7POINT1POINT4
    {14, AudioChannelLayout::CH_9POINT1POINT4},  // 14: CH_9POINT1POINT4
    {16, AudioChannelLayout::CH_9POINT1POINT6},  // 16: CH_9POINT1POINT6
    {24, AudioChannelLayout::CH_22POINT2},       // 24: CH_22POINT2
};

const std::vector<std::pair<AVSampleFormat, AudioSampleFormat>> g_pFfSampleFmtMap = {
    {AVSampleFormat::AV_SAMPLE_FMT_U8, AudioSampleFormat::SAMPLE_U8},
    {AVSampleFormat::AV_SAMPLE_FMT_S16, AudioSampleFormat::SAMPLE_S16LE},
    {AVSampleFormat::AV_SAMPLE_FMT_S32, AudioSampleFormat::SAMPLE_S32LE},
    {AVSampleFormat::AV_SAMPLE_FMT_FLT, AudioSampleFormat::SAMPLE_F32LE},
    {AVSampleFormat::AV_SAMPLE_FMT_U8P, AudioSampleFormat::SAMPLE_U8P},
    {AVSampleFormat::AV_SAMPLE_FMT_S16P, AudioSampleFormat::SAMPLE_S16P},
    {AVSampleFormat::AV_SAMPLE_FMT_S32P, AudioSampleFormat::SAMPLE_S32P},
    {AVSampleFormat::AV_SAMPLE_FMT_FLTP, AudioSampleFormat::SAMPLE_F32P},
};

// align with player framework capability.
const std::vector<std::pair<AVCodecID, AudioSampleFormat>> g_pFfCodeIDToSampleFmtMap = {
    {AVCodecID::AV_CODEC_ID_PCM_U8, AudioSampleFormat::SAMPLE_U8},
    {AVCodecID::AV_CODEC_ID_PCM_S16LE, AudioSampleFormat::SAMPLE_S16LE},
    {AVCodecID::AV_CODEC_ID_PCM_S24LE, AudioSampleFormat::SAMPLE_S24LE},
    {AVCodecID::AV_CODEC_ID_PCM_S32LE, AudioSampleFormat::SAMPLE_S32LE},
    {AVCodecID::AV_CODEC_ID_PCM_F32LE, AudioSampleFormat::SAMPLE_F32LE},
    {AVCodecID::AV_CODEC_ID_PCM_S16BE, AudioSampleFormat::SAMPLE_S16BE},
    {AVCodecID::AV_CODEC_ID_PCM_S24BE, AudioSampleFormat::SAMPLE_S24BE},
    {AVCodecID::AV_CODEC_ID_PCM_S32BE, AudioSampleFormat::SAMPLE_S32BE},
    {AVCodecID::AV_CODEC_ID_PCM_F32BE, AudioSampleFormat::SAMPLE_F32BE},
    {AVCodecID::AV_CODEC_ID_PCM_F64BE, AudioSampleFormat::SAMPLE_F64BE},
    {AVCodecID::AV_CODEC_ID_PCM_S8, AudioSampleFormat::SAMPLE_S8},
    {AVCodecID::AV_CODEC_ID_PCM_F64LE, AudioSampleFormat::SAMPLE_F64LE},
    {AVCodecID::AV_CODEC_ID_PCM_S64LE, AudioSampleFormat::SAMPLE_S64LE},
    {AVCodecID::AV_CODEC_ID_PCM_S8_PLANAR, AudioSampleFormat::SAMPLE_S8P},
    {AVCodecID::AV_CODEC_ID_PCM_S16LE_PLANAR, AudioSampleFormat::SAMPLE_S16LEP},
    {AVCodecID::AV_CODEC_ID_PCM_S16BE_PLANAR, AudioSampleFormat::SAMPLE_S16BEP},
    {AVCodecID::AV_CODEC_ID_PCM_S24LE_PLANAR, AudioSampleFormat::SAMPLE_S24LEP},
    {AVCodecID::AV_CODEC_ID_PCM_S32LE_PLANAR, AudioSampleFormat::SAMPLE_S32LEP},
    {AVCodecID::AV_CODEC_ID_PCM_DVD, AudioSampleFormat::SAMPLE_DVD},
    {AVCodecID::AV_CODEC_ID_PCM_BLURAY, AudioSampleFormat::SAMPLE_BLURAY},
    {AVCodecID::AV_CODEC_ID_PCM_ALAW, AudioSampleFormat::SAMPLE_S16LE},
    {AVCodecID::AV_CODEC_ID_PCM_MULAW, AudioSampleFormat::SAMPLE_S16LE},
};

const std::vector<std::pair<AudioChannelLayout, std::string_view>> g_ChannelLayoutToString = {
    {AudioChannelLayout::UNKNOWN, "UNKNOW"},
    {AudioChannelLayout::MONO, "MONO"},
    {AudioChannelLayout::STEREO, "STEREO"},
    {AudioChannelLayout::CH_2POINT1, "2POINT1"},
    {AudioChannelLayout::CH_2_1, "CH_2_1"},
    {AudioChannelLayout::SURROUND, "SURROUND"},
    {AudioChannelLayout::CH_3POINT1, "3POINT1"},
    {AudioChannelLayout::CH_4POINT0, "4POINT0"},
    {AudioChannelLayout::CH_4POINT1, "4POINT1"},
    {AudioChannelLayout::CH_2_2, "CH_2_2"},
    {AudioChannelLayout::QUAD, "QUAD"},
    {AudioChannelLayout::CH_5POINT0, "5POINT0"},
    {AudioChannelLayout::CH_5POINT1, "5POINT1"},
    {AudioChannelLayout::CH_5POINT0_BACK, "5POINT0_BACK"},
    {AudioChannelLayout::CH_5POINT1_BACK, "5POINT1_BACK"},
    {AudioChannelLayout::CH_6POINT0, "6POINT0"},
    {AudioChannelLayout::CH_6POINT0_FRONT, "6POINT0_FRONT"},
    {AudioChannelLayout::HEXAGONAL, "HEXAGONAL"},
    {AudioChannelLayout::CH_6POINT1, "6POINT1"},
    {AudioChannelLayout::CH_6POINT1_BACK, "6POINT1_BACK"},
    {AudioChannelLayout::CH_6POINT1_FRONT, "6POINT1_FRONT"},
    {AudioChannelLayout::CH_7POINT0, "7POINT0"},
    {AudioChannelLayout::CH_7POINT0_FRONT, "7POINT0_FRONT"},
    {AudioChannelLayout::CH_7POINT1, "7POINT1"},
    {AudioChannelLayout::CH_7POINT1_WIDE, "7POINT1_WIDE"},
    {AudioChannelLayout::CH_7POINT1_WIDE_BACK, "7POINT1_WIDE_BACK"},
    {AudioChannelLayout::CH_3POINT1POINT2, "CH_3POINT1POINT2"},
    {AudioChannelLayout::CH_5POINT1POINT2, "CH_5POINT1POINT2"},
    {AudioChannelLayout::CH_5POINT1POINT4, "CH_5POINT1POINT4"},
    {AudioChannelLayout::CH_7POINT1POINT2, "CH_7POINT1POINT2"},
    {AudioChannelLayout::CH_7POINT1POINT4, "CH_7POINT1POINT4"},
    {AudioChannelLayout::CH_9POINT1POINT4, "CH_9POINT1POINT4"},
    {AudioChannelLayout::CH_9POINT1POINT6, "CH_9POINT1POINT6"},
    {AudioChannelLayout::CH_10POINT2, "CH_10POINT2"},
    {AudioChannelLayout::CH_22POINT2, "CH_22POINT2"},
    {AudioChannelLayout::OCTAGONAL, "OCTAGONAL"},
    {AudioChannelLayout::HEXADECAGONAL, "HEXADECAGONAL"},
    {AudioChannelLayout::STEREO_DOWNMIX, "STEREO_DOWNMIX"},
    {AudioChannelLayout::CH_2POINT0POINT2, "CH_2POINT0POINT2"},
    {AudioChannelLayout::CH_2POINT1POINT2, "CH_2POINT1POINT2"},
    {AudioChannelLayout::CH_3POINT0POINT2, "CH_3POINT0POINT2"},
    {AudioChannelLayout::HOA_ORDER1_ACN_N3D, "HOA_ORDER1_ACN_N3D"},
    {AudioChannelLayout::HOA_ORDER1_ACN_SN3D, "HOA_ORDER1_ACN_SN3D"},
    {AudioChannelLayout::HOA_ORDER1_FUMA, "HOA_ORDER1_FUMA"},
    {AudioChannelLayout::HOA_ORDER2_ACN_N3D, "HOA_ORDER2_ACN_N3D"},
    {AudioChannelLayout::HOA_ORDER2_ACN_SN3D, "HOA_ORDER2_ACN_SN3D"},
    {AudioChannelLayout::HOA_ORDER2_FUMA, "HOA_ORDER2_FUMA"},
    {AudioChannelLayout::HOA_ORDER3_ACN_N3D, "HOA_ORDER3_ACN_N3D"},
    {AudioChannelLayout::HOA_ORDER3_ACN_SN3D, "HOA_ORDER3_ACN_SN3D"},
    {AudioChannelLayout::HOA_ORDER3_FUMA, "HOA_ORDER3_FUMA"},
};

const std::vector<std::pair<AVColorPrimaries, ColorPrimary>> g_pFfColorPrimariesMap = {
    {AVColorPrimaries::AVCOL_PRI_BT709, ColorPrimary::BT709},
    {AVColorPrimaries::AVCOL_PRI_UNSPECIFIED, ColorPrimary::UNSPECIFIED},
    {AVColorPrimaries::AVCOL_PRI_BT470M, ColorPrimary::BT470_M},
    {AVColorPrimaries::AVCOL_PRI_BT470BG, ColorPrimary::BT601_625},
    {AVColorPrimaries::AVCOL_PRI_SMPTE170M, ColorPrimary::BT601_525},
    {AVColorPrimaries::AVCOL_PRI_SMPTE240M, ColorPrimary::SMPTE_ST240},
    {AVColorPrimaries::AVCOL_PRI_FILM, ColorPrimary::GENERIC_FILM},
    {AVColorPrimaries::AVCOL_PRI_BT2020, ColorPrimary::BT2020},
    {AVColorPrimaries::AVCOL_PRI_SMPTE428, ColorPrimary::SMPTE_ST428},
    {AVColorPrimaries::AVCOL_PRI_SMPTEST428_1, ColorPrimary::SMPTE_ST428},
    {AVColorPrimaries::AVCOL_PRI_SMPTE431, ColorPrimary::P3DCI},
    {AVColorPrimaries::AVCOL_PRI_SMPTE432, ColorPrimary::P3D65},
};

const std::vector<std::pair<AVColorTransferCharacteristic, TransferCharacteristic>> g_pFfTransferCharacteristicMap = {
    {AVColorTransferCharacteristic::AVCOL_TRC_BT709, TransferCharacteristic::BT709},
    {AVColorTransferCharacteristic::AVCOL_TRC_UNSPECIFIED, TransferCharacteristic::UNSPECIFIED},
    {AVColorTransferCharacteristic::AVCOL_TRC_GAMMA22, TransferCharacteristic::GAMMA_2_2},
    {AVColorTransferCharacteristic::AVCOL_TRC_GAMMA28, TransferCharacteristic::GAMMA_2_8},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE170M, TransferCharacteristic::BT601},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE240M, TransferCharacteristic::SMPTE_ST240},
    {AVColorTransferCharacteristic::AVCOL_TRC_LINEAR, TransferCharacteristic::LINEAR},
    {AVColorTransferCharacteristic::AVCOL_TRC_LOG, TransferCharacteristic::LOG},
    {AVColorTransferCharacteristic::AVCOL_TRC_LOG_SQRT, TransferCharacteristic::LOG_SQRT},
    {AVColorTransferCharacteristic::AVCOL_TRC_IEC61966_2_4, TransferCharacteristic::IEC_61966_2_4},
    {AVColorTransferCharacteristic::AVCOL_TRC_BT1361_ECG, TransferCharacteristic::BT1361},
    {AVColorTransferCharacteristic::AVCOL_TRC_IEC61966_2_1, TransferCharacteristic::IEC_61966_2_1},
    {AVColorTransferCharacteristic::AVCOL_TRC_BT2020_10, TransferCharacteristic::BT2020_10BIT},
    {AVColorTransferCharacteristic::AVCOL_TRC_BT2020_12, TransferCharacteristic::BT2020_12BIT},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE2084, TransferCharacteristic::PQ},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTEST2084, TransferCharacteristic::PQ},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE428, TransferCharacteristic::SMPTE_ST428},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTEST428_1, TransferCharacteristic::SMPTE_ST428},
    {AVColorTransferCharacteristic::AVCOL_TRC_ARIB_STD_B67, TransferCharacteristic::HLG},
};

const std::vector<std::pair<AVColorSpace, MatrixCoefficient>> g_pFfMatrixCoefficientMap = {
    {AVColorSpace::AVCOL_SPC_RGB, MatrixCoefficient::IDENTITY},
    {AVColorSpace::AVCOL_SPC_BT709, MatrixCoefficient::BT709},
    {AVColorSpace::AVCOL_SPC_UNSPECIFIED, MatrixCoefficient::UNSPECIFIED},
    {AVColorSpace::AVCOL_SPC_FCC, MatrixCoefficient::FCC},
    {AVColorSpace::AVCOL_SPC_BT470BG, MatrixCoefficient::BT601_625},
    {AVColorSpace::AVCOL_SPC_SMPTE170M, MatrixCoefficient::BT601_525},
    {AVColorSpace::AVCOL_SPC_SMPTE240M, MatrixCoefficient::SMPTE_ST240},
    {AVColorSpace::AVCOL_SPC_YCGCO, MatrixCoefficient::YCGCO},
    {AVColorSpace::AVCOL_SPC_YCOCG, MatrixCoefficient::YCGCO},
    {AVColorSpace::AVCOL_SPC_BT2020_NCL, MatrixCoefficient::BT2020_NCL},
    {AVColorSpace::AVCOL_SPC_BT2020_CL, MatrixCoefficient::BT2020_CL},
    {AVColorSpace::AVCOL_SPC_SMPTE2085, MatrixCoefficient::SMPTE_ST2085},
    {AVColorSpace::AVCOL_SPC_CHROMA_DERIVED_NCL, MatrixCoefficient::CHROMATICITY_NCL},
    {AVColorSpace::AVCOL_SPC_CHROMA_DERIVED_CL, MatrixCoefficient::CHROMATICITY_CL},
    {AVColorSpace::AVCOL_SPC_ICTCP, MatrixCoefficient::ICTCP},
};

const std::vector<std::pair<AVColorRange, int>> g_pFfColorRangeMap = {
    {AVColorRange::AVCOL_RANGE_MPEG, 0},
    {AVColorRange::AVCOL_RANGE_JPEG, 1},
};

const std::vector<std::pair<AVChromaLocation, ChromaLocation>> g_pFfChromaLocationMap = {
    {AVChromaLocation::AVCHROMA_LOC_UNSPECIFIED, ChromaLocation::UNSPECIFIED},
    {AVChromaLocation::AVCHROMA_LOC_LEFT, ChromaLocation::LEFT},
    {AVChromaLocation::AVCHROMA_LOC_CENTER, ChromaLocation::CENTER},
    {AVChromaLocation::AVCHROMA_LOC_TOPLEFT, ChromaLocation::TOPLEFT},
    {AVChromaLocation::AVCHROMA_LOC_TOP, ChromaLocation::TOP},
    {AVChromaLocation::AVCHROMA_LOC_BOTTOMLEFT, ChromaLocation::BOTTOMLEFT},
    {AVChromaLocation::AVCHROMA_LOC_BOTTOM, ChromaLocation::BOTTOM},
};

const std::vector<std::pair<int, HEVCProfile>> g_pFfHEVCProfileMap = {
    {FF_PROFILE_HEVC_MAIN, HEVCProfile::HEVC_PROFILE_MAIN},
    {FF_PROFILE_HEVC_MAIN_10, HEVCProfile::HEVC_PROFILE_MAIN_10},
    {FF_PROFILE_HEVC_MAIN_STILL_PICTURE, HEVCProfile::HEVC_PROFILE_MAIN_STILL},
};

const std::vector<std::pair<int, HEVCLevel>> g_pFfHEVCLevelMap = {
    {30, HEVCLevel::HEVC_LEVEL_1},   {60, HEVCLevel::HEVC_LEVEL_2},  {63, HEVCLevel::HEVC_LEVEL_21},
    {90, HEVCLevel::HEVC_LEVEL_3},   {93, HEVCLevel::HEVC_LEVEL_31}, {120, HEVCLevel::HEVC_LEVEL_4},
    {123, HEVCLevel::HEVC_LEVEL_41}, {150, HEVCLevel::HEVC_LEVEL_5}, {153, HEVCLevel::HEVC_LEVEL_51},
    {156, HEVCLevel::HEVC_LEVEL_52}, {180, HEVCLevel::HEVC_LEVEL_6}, {183, HEVCLevel::HEVC_LEVEL_61},
    {186, HEVCLevel::HEVC_LEVEL_62},
};

HEVCLevel Converter::ConvertToOHHEVCLevel(int ffHEVCLevel)
{
    auto ite = std::find_if(g_pFfHEVCLevelMap.begin(), g_pFfHEVCLevelMap.end(),
                            [&ffHEVCLevel](const auto &item) -> bool { return item.first == ffHEVCLevel; });
    if (ite == g_pFfHEVCLevelMap.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_D32, ffHEVCLevel);
        return HEVCLevel::HEVC_LEVEL_UNKNOW;
    }
    return ite->second;
}

HEVCProfile Converter::ConvertToOHHEVCProfile(int ffHEVCProfile)
{
    auto ite = std::find_if(g_pFfHEVCProfileMap.begin(), g_pFfHEVCProfileMap.end(),
                            [&ffHEVCProfile](const auto &item) -> bool { return item.first == ffHEVCProfile; });
    if (ite == g_pFfHEVCProfileMap.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_D32, ffHEVCProfile);
        return HEVCProfile::HEVC_PROFILE_UNKNOW;
    }
    return ite->second;
}

ColorPrimary Converter::ConvertFFMpegToOHColorPrimaries(AVColorPrimaries ffColorPrimaries)
{
    auto ite = std::find_if(g_pFfColorPrimariesMap.begin(), g_pFfColorPrimariesMap.end(),
                            [&ffColorPrimaries](const auto &item) -> bool { return item.first == ffColorPrimaries; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfColorPrimariesMap.end(), ColorPrimary::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(ffColorPrimaries));
    return ite->second;
}

TransferCharacteristic Converter::ConvertFFMpegToOHColorTrans(AVColorTransferCharacteristic ffColorTrans)
{
    auto ite = std::find_if(g_pFfTransferCharacteristicMap.begin(), g_pFfTransferCharacteristicMap.end(),
                            [&ffColorTrans](const auto &item) -> bool { return item.first == ffColorTrans; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfTransferCharacteristicMap.end(), TransferCharacteristic::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(ffColorTrans));
    return ite->second;
}

MatrixCoefficient Converter::ConvertFFMpegToOHColorMatrix(AVColorSpace ffColorSpace)
{
    auto ite = std::find_if(g_pFfMatrixCoefficientMap.begin(), g_pFfMatrixCoefficientMap.end(),
                            [&ffColorSpace](const auto &item) -> bool { return item.first == ffColorSpace; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfMatrixCoefficientMap.end(), MatrixCoefficient::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(ffColorSpace));
    return ite->second;
}

int Converter::ConvertFFMpegToOHColorRange(AVColorRange ffColorRange)
{
    auto ite = std::find_if(g_pFfColorRangeMap.begin(), g_pFfColorRangeMap.end(),
                            [&ffColorRange](const auto &item) -> bool { return item.first == ffColorRange; });
    if (ite == g_pFfColorRangeMap.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(ffColorRange));
        return 0;
    }
    return ite->second;
}

ChromaLocation Converter::ConvertFFMpegToOHChromaLocation(AVChromaLocation ffChromaLocation)
{
    auto ite = std::find_if(g_pFfChromaLocationMap.begin(), g_pFfChromaLocationMap.end(),
                            [&ffChromaLocation](const auto &item) -> bool { return item.first == ffChromaLocation; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfChromaLocationMap.end(), ChromaLocation::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(ffChromaLocation));
    return ite->second;
}

AudioSampleFormat Converter::ConvertFFMpegAVCodecIdToOHAudioFormat(AVCodecID codecId)
{
    auto ite = std::find_if(g_pFfCodeIDToSampleFmtMap.begin(), g_pFfCodeIDToSampleFmtMap.end(),
                            [&codecId](const auto &item) -> bool { return item.first == codecId; });
    if (ite == g_pFfCodeIDToSampleFmtMap.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(codecId));
        return AudioSampleFormat::INVALID_WIDTH;
    }
    return ite->second;
}

AudioSampleFormat Converter::ConvertFFMpegToOHAudioFormat(AVSampleFormat ffSampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&ffSampleFormat](const auto &item) -> bool { return item.first == ffSampleFormat; });
    if (ite == g_pFfSampleFmtMap.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(ffSampleFormat));
        return AudioSampleFormat::INVALID_WIDTH;
    }
    return ite->second;
}

AudioChannelLayout Converter::GetDefaultChannelLayout(int channels)
{
    AudioChannelLayout layout = AudioChannelLayout::MONO;
    auto ite = std::find_if(g_channelLayoutDefaultMap.begin(), g_channelLayoutDefaultMap.end(),
                            [&channels](const auto &item) -> bool { return item.first == channels; });
    if (ite != g_channelLayoutDefaultMap.end()) {
        layout = ite->second;
    }
    MEDIA_LOG_W("Default: " PUBLIC_LOG_S, ConvertOHAudioChannelLayoutToString(layout).data());
    return layout;
}

AudioChannelLayout Converter::ConvertFFToOHAudioChannelLayoutV2(uint64_t ffChannelLayout, int channels)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&ffChannelLayout](const auto &item) -> bool { return item.second == ffChannelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_U64, ffChannelLayout);
        return GetDefaultChannelLayout(channels);
    }
    return ite->first;
}

std::string_view Converter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout)
{
    auto ite = std::find_if(g_ChannelLayoutToString.begin(), g_ChannelLayoutToString.end(),
                            [&layout](const auto &item) -> bool { return item.first == layout; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_ChannelLayoutToString.end(), g_ChannelLayoutToString[0].second,
                             "Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(layout));
    return ite->second;
}

AudioChannelLayout Converter::ConvertAudioVividToOHAudioChannelLayout(uint64_t ffChannelLayout, int channels)
{
    auto ite = std::find_if(g_audioVividChannelLayoutMap.begin(), g_audioVividChannelLayoutMap.end(),
                            [&ffChannelLayout](const auto &item) -> bool {
                                return static_cast<uint64_t>(item.first) == ffChannelLayout;
                            });
    CHECK_AND_RETURN_RET_LOGW((ite != g_audioVividChannelLayoutMap.end()) && (ite -> second == channels),
                             GetDefaultChannelLayout(channels), "Convert channel layout failed: " PUBLIC_LOG_U64,
                             ffChannelLayout);
    return ite->first;
}

void Converter::ParseColorBoxInfo(HevcParseFormat parse, Meta &format)
{
    ColorPrimary colorPrimaries = ConvertFFMpegToOHColorPrimaries(
        static_cast<AVColorPrimaries>(parse.colorPrimaries));
    if (colorPrimaries != ColorPrimary::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);
        format.Set<Tag::VIDEO_COLOR_RANGE>((bool)(parse.colorRange));
    } else {
        MEDIA_LOG_D("Parse colorPri from xps failed");
    }

    TransferCharacteristic colorTrans = ConvertFFMpegToOHColorTrans(
        static_cast<AVColorTransferCharacteristic>(parse.colorTransfer));
    if (colorTrans != TransferCharacteristic::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_TRC>(colorTrans);
    } else {
        MEDIA_LOG_D("Parse colorTrc from xps failed");
    }

    MatrixCoefficient colorMatrix = ConvertFFMpegToOHColorMatrix(
        static_cast<AVColorSpace>(parse.colorMatrixCoeff));
    if (colorMatrix != MatrixCoefficient::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix);
    } else {
        MEDIA_LOG_D("Parse colorMatrix from xps failed");
    }

    ChromaLocation chromaLoc = ConvertFFMpegToOHChromaLocation(
        static_cast<AVChromaLocation>(parse.chromaLocation));
    if (chromaLoc != ChromaLocation::UNSPECIFIED) {
        format.Set<Tag::VIDEO_CHROMA_LOCATION>(chromaLoc);
    } else {
        MEDIA_LOG_D("Parse chromaLoc from xps failed");
    }
}

void Converter::ParseHdrTypeInfo(HdrBoxInfo hdrBoxInfo, Meta &format, HevcParseFormat parse)
{
    format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::NONE);

    ColorPrimary colorPrimaries = ColorPrimary::UNSPECIFIED;
    format.Get<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);
    FALSE_RETURN_NOLOG(colorPrimaries == ColorPrimary::BT2020);

    MatrixCoefficient colorMatrix = MatrixCoefficient::UNSPECIFIED;
    format.Get<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix);
    FALSE_RETURN_NOLOG(colorMatrix == MatrixCoefficient::BT2020_CL || colorMatrix == MatrixCoefficient::BT2020_NCL);

    if (parse.isHdrVivid) {
        format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::HDR_VIVID);
        return;
    } else if (parse.isHdr10Plus) {
        format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::HDR_10);
        return;
    }

    if (hdrBoxInfo.haveHdrDoblyVisionBox) {
        format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::HDR_10);
        return;
    } else if (hdrBoxInfo.haveHdrVividBox) {
        format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::HDR_VIVID);
        return;
    }

    TransferCharacteristic colorTrans = TransferCharacteristic::UNSPECIFIED;
    format.Get<Tag::VIDEO_COLOR_TRC>(colorTrans);
    if (colorTrans == TransferCharacteristic::HLG) {
        format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::HLG);
    } else if (colorTrans == TransferCharacteristic::PQ) {
        format.Set<Tag::VIDEO_HDR_TYPE>(HDRType::HDR_10);
    }
}

std::string Converter::ToLower(const std::string& str)
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

bool Converter::IsUTF8(const std::string &data)
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

std::string Converter::ConvertGBKToUTF8(const std::string &strGbk)
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

bool Converter::IsGBK(const char* data)
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
} // namespace Plugins
} // namespace Media
} // namespace OHOS