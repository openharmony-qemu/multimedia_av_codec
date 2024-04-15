/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "FfmpegConverter"

#include <vector>
#include <algorithm>
#include "common/log.h"
#include "ffmpeg_converter.h"
namespace {
constexpr int US_PER_SECOND = 1000000;
}
namespace OHOS {
namespace Media {
namespace Plugins {
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

const std::vector<std::pair<int, AudioChannelLayout>> g_channelLayoutDefaukltMap = {
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

HEVCLevel FFMpegConverter::ConvertFFMpegToOHHEVCLevel(int ffHEVCLevel)
{
    auto ite = std::find_if(g_pFfHEVCLevelMap.begin(), g_pFfHEVCLevelMap.end(),
                            [&ffHEVCLevel](const auto &item) -> bool { return item.first == ffHEVCLevel; });
    if (ite == g_pFfHEVCLevelMap.end()) {
        MEDIA_LOG_W("Convert hevc level failed: " PUBLIC_LOG_D32 "", ffHEVCLevel);
        return HEVCLevel::HEVC_LEVEL_UNKNOW;
    }
    return ite->second;
}

HEVCProfile FFMpegConverter::ConvertFFMpegToOHHEVCProfile(int ffHEVCProfile)
{
    auto ite = std::find_if(g_pFfHEVCProfileMap.begin(), g_pFfHEVCProfileMap.end(),
                            [&ffHEVCProfile](const auto &item) -> bool { return item.first == ffHEVCProfile; });
    if (ite == g_pFfHEVCProfileMap.end()) {
        MEDIA_LOG_W("Convert hevc profile failed: " PUBLIC_LOG_D32 "", ffHEVCProfile);
        return HEVCProfile::HEVC_PROFILE_UNKNOW;
    }
    return ite->second;
}

ColorPrimary FFMpegConverter::ConvertFFMpegToOHColorPrimaries(AVColorPrimaries ffColorPrimaries)
{
    auto ite = std::find_if(g_pFfColorPrimariesMap.begin(), g_pFfColorPrimariesMap.end(),
                            [&ffColorPrimaries](const auto &item) -> bool { return item.first == ffColorPrimaries; });
    if (ite == g_pFfColorPrimariesMap.end()) {
        MEDIA_LOG_W("Convert color primaries failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(ffColorPrimaries));
        return ColorPrimary::UNSPECIFIED;
    }
    return ite->second;
}

TransferCharacteristic FFMpegConverter::ConvertFFMpegToOHColorTrans(AVColorTransferCharacteristic ffColorTrans)
{
    auto ite = std::find_if(g_pFfTransferCharacteristicMap.begin(), g_pFfTransferCharacteristicMap.end(),
                            [&ffColorTrans](const auto &item) -> bool { return item.first == ffColorTrans; });
    if (ite == g_pFfTransferCharacteristicMap.end()) {
        MEDIA_LOG_W("Convert color trans failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(ffColorTrans));
        return TransferCharacteristic::UNSPECIFIED;
    }
    return ite->second;
}

MatrixCoefficient FFMpegConverter::ConvertFFMpegToOHColorMatrix(AVColorSpace ffColorSpace)
{
    auto ite = std::find_if(g_pFfMatrixCoefficientMap.begin(), g_pFfMatrixCoefficientMap.end(),
                            [&ffColorSpace](const auto &item) -> bool { return item.first == ffColorSpace; });
    if (ite == g_pFfMatrixCoefficientMap.end()) {
        MEDIA_LOG_W("Convert color matrix failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(ffColorSpace));
        return MatrixCoefficient::UNSPECIFIED;
    }
    return ite->second;
}

int FFMpegConverter::ConvertFFMpegToOHColorRange(AVColorRange ffColorRange)
{
    auto ite = std::find_if(g_pFfColorRangeMap.begin(), g_pFfColorRangeMap.end(),
                            [&ffColorRange](const auto &item) -> bool { return item.first == ffColorRange; });
    if (ite == g_pFfColorRangeMap.end()) {
        MEDIA_LOG_W("Convert color range failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(ffColorRange));
        return 0;
    }
    return ite->second;
}

ChromaLocation FFMpegConverter::ConvertFFMpegToOHChromaLocation(AVChromaLocation ffChromaLocation)
{
    auto ite = std::find_if(g_pFfChromaLocationMap.begin(), g_pFfChromaLocationMap.end(),
                            [&ffChromaLocation](const auto &item) -> bool { return item.first == ffChromaLocation; });
    if (ite == g_pFfChromaLocationMap.end()) {
        MEDIA_LOG_W("Convert chroma location failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(ffChromaLocation));
        return ChromaLocation::UNSPECIFIED;
    }
    return ite->second;
}

AudioSampleFormat FFMpegConverter::ConvertFFMpegAVCodecIdToOHAudioFormat(AVCodecID codecId)
{
    auto ite = std::find_if(g_pFfCodeIDToSampleFmtMap.begin(), g_pFfCodeIDToSampleFmtMap.end(),
                            [&codecId](const auto &item) -> bool { return item.first == codecId; });
    if (ite == g_pFfCodeIDToSampleFmtMap.end()) {
        MEDIA_LOG_W("Convert codec id failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(codecId));
        return AudioSampleFormat::INVALID_WIDTH;
    }
    return ite->second;
}

AudioSampleFormat FFMpegConverter::ConvertFFMpegToOHAudioFormat(AVSampleFormat ffSampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&ffSampleFormat](const auto &item) -> bool { return item.first == ffSampleFormat; });
    if (ite == g_pFfSampleFmtMap.end()) {
        MEDIA_LOG_W("Convert sample format failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(ffSampleFormat));
        return AudioSampleFormat::INVALID_WIDTH;
    }
    return ite->second;
}

AVSampleFormat FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat sampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&sampleFormat](const auto &item) -> bool { return item.second == sampleFormat; });
    if (ite == g_pFfSampleFmtMap.end()) {
        MEDIA_LOG_W("Convert sample format failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(sampleFormat));
        return AVSampleFormat::AV_SAMPLE_FMT_NONE;
    }
    return ite->first;
}

AudioChannelLayout FFMpegConverter::ConvertFFToOHAudioChannelLayout(uint64_t ffChannelLayout)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&ffChannelLayout](const auto &item) -> bool { return item.second == ffChannelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        MEDIA_LOG_W("Convert channel layout failed: " PUBLIC_LOG_U64, ffChannelLayout);
        return AudioChannelLayout::MONO;
    }
    return ite->first;
}

AudioChannelLayout FFMpegConverter::GetDefaultChannelLayout(int channels)
{
    AudioChannelLayout layout = AudioChannelLayout::MONO;
    auto ite = std::find_if(g_channelLayoutDefaukltMap.begin(), g_channelLayoutDefaukltMap.end(),
                            [&channels](const auto &item) -> bool { return item.first == channels; });
    if (ite != g_channelLayoutDefaukltMap.end()) {
        layout = ite->second;
    }
    MEDIA_LOG_W("Get default channel layout: " PUBLIC_LOG_S "", ConvertOHAudioChannelLayoutToString(layout).data());
    return layout;
}

AudioChannelLayout FFMpegConverter::ConvertFFToOHAudioChannelLayoutV2(uint64_t ffChannelLayout, int channels)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&ffChannelLayout](const auto &item) -> bool { return item.second == ffChannelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        MEDIA_LOG_W("Convert channel layout failed: " PUBLIC_LOG_U64, ffChannelLayout);
        return GetDefaultChannelLayout(channels);
    }
    return ite->first;
}

uint64_t FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout channelLayout)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&channelLayout](const auto &item) -> bool { return item.first == channelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        MEDIA_LOG_W("Convert channel layout failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(channelLayout));
        return AV_CH_LAYOUT_NATIVE;
    }
    return ite->second;
}

std::string_view FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout)
{
    auto ite = std::find_if(g_ChannelLayoutToString.begin(), g_ChannelLayoutToString.end(),
                            [&layout](const auto &item) -> bool { return item.first == layout; });
    if (ite == g_ChannelLayoutToString.end()) {
        MEDIA_LOG_W("Convert channel layout failed: " PUBLIC_LOG_D32 "", static_cast<int32_t>(layout));
        return g_ChannelLayoutToString[0].second;
    }
    return ite->second;
}

int64_t FFMpegConverter::ConvertAudioPtsToUs(int64_t pts, AVRational base)
{
    if (pts == AV_NOPTS_VALUE) {
        return -1;
    }
    AVRational us = {1, US_PER_SECOND};
    return av_rescale_q(pts, base, us);
}

std::string FFMpegConverter::AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS