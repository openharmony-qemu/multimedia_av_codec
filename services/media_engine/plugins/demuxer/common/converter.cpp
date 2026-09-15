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
#include <limits>
#include <new>
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

#ifdef USE_FF_DEMUXER
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
    {AudioChannelLayout::CH_5POINT0, 5},
    {AudioChannelLayout::CH_5POINT1, 6},
    {AudioChannelLayout::CH_7POINT0, 7},
    {AudioChannelLayout::CH_7POINT1, 8},
    {AudioChannelLayout::CH_5POINT1POINT2, 8},
    {AudioChannelLayout::CH_5POINT1POINT4, 10},
    {AudioChannelLayout::CH_7POINT1POINT2, 10},
    {AudioChannelLayout::CH_7POINT1POINT4, 12},
    {AudioChannelLayout::HOA_ORDER1_ACN_SN3D, 4},
    {AudioChannelLayout::HOA_ORDER2_ACN_SN3D, 9},
    {AudioChannelLayout::HOA_ORDER3_ACN_SN3D, 16},
};
#endif

const std::vector<std::pair<int, AudioChannelLayout>> g_channelLayoutDefaultMap = {
    {2, AudioChannelLayout::STEREO},             // 2: STEREO
    {4, AudioChannelLayout::CH_4POINT0},         // 4: CH_4POINT0
    {5, AudioChannelLayout::CH_5POINT0},         // 5: CH_5POINT0
    {6, AudioChannelLayout::CH_5POINT1},         // 6: CH_5POINT1
    {7, AudioChannelLayout::CH_7POINT0},         // 7: CH_7POINT0
    {8, AudioChannelLayout::CH_5POINT1POINT2},   // 8: CH_5POINT1POINT2
    {9, AudioChannelLayout::HOA_ORDER2_ACN_N3D}, // 9: HOA_ORDER2_ACN_N3D
    {10, AudioChannelLayout::CH_7POINT1POINT2},  // 10: CH_7POINT1POINT2 or CH_5POINT1POINT4 ?
    {12, AudioChannelLayout::CH_7POINT1POINT4},  // 12: CH_7POINT1POINT4
    {14, AudioChannelLayout::CH_9POINT1POINT4},  // 14: CH_9POINT1POINT4
    {16, AudioChannelLayout::CH_9POINT1POINT6},  // 16: CH_9POINT1POINT6
    {24, AudioChannelLayout::CH_22POINT2},       // 24: CH_22POINT2
};

#ifdef USE_FF_DEMUXER
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
#endif

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

const std::vector<std::pair<int32_t, ColorPrimary>> g_pFfColorPrimariesMap = {
    {1, ColorPrimary::BT709},
    {2, ColorPrimary::UNSPECIFIED},
    {4, ColorPrimary::BT470_M},
    {5, ColorPrimary::BT601_625},
    {6, ColorPrimary::BT601_525},
    {7, ColorPrimary::SMPTE_ST240},
    {8, ColorPrimary::GENERIC_FILM},
    {9, ColorPrimary::BT2020},
    {10, ColorPrimary::SMPTE_ST428},
    {11, ColorPrimary::P3DCI},
    {12, ColorPrimary::P3D65},
};

const std::vector<std::pair<int32_t, TransferCharacteristic>> g_pFfTransferCharacteristicMap = {
    {1, TransferCharacteristic::BT709},
    {2, TransferCharacteristic::UNSPECIFIED},
    {4, TransferCharacteristic::GAMMA_2_2},
    {5, TransferCharacteristic::GAMMA_2_8},
    {6, TransferCharacteristic::BT601},
    {7, TransferCharacteristic::SMPTE_ST240},
    {8, TransferCharacteristic::LINEAR},
    {9, TransferCharacteristic::LOG},
    {10, TransferCharacteristic::LOG_SQRT},
    {11, TransferCharacteristic::IEC_61966_2_4},
    {12, TransferCharacteristic::BT1361},
    {13, TransferCharacteristic::IEC_61966_2_1},
    {14, TransferCharacteristic::BT2020_10BIT},
    {15, TransferCharacteristic::BT2020_12BIT},
    {16, TransferCharacteristic::PQ},
    {17, TransferCharacteristic::SMPTE_ST428},
    {18, TransferCharacteristic::HLG},
};

const std::vector<std::pair<int32_t, MatrixCoefficient>> g_pFfMatrixCoefficientMap = {
    {0, MatrixCoefficient::IDENTITY},
    {1, MatrixCoefficient::BT709},
    {2, MatrixCoefficient::UNSPECIFIED},
    {4, MatrixCoefficient::FCC},
    {5, MatrixCoefficient::BT601_625},
    {6, MatrixCoefficient::BT601_525},
    {7, MatrixCoefficient::SMPTE_ST240},
    {8, MatrixCoefficient::YCGCO},
    {9, MatrixCoefficient::BT2020_NCL},
    {10, MatrixCoefficient::BT2020_CL},
    {11, MatrixCoefficient::SMPTE_ST2085},
    {12, MatrixCoefficient::CHROMATICITY_NCL},
    {13, MatrixCoefficient::CHROMATICITY_CL},
    {14, MatrixCoefficient::ICTCP},
};

const std::vector<std::pair<int32_t, int32_t>> g_pFfColorRangeMap = {
    {1, 0},
    {2, 1},
};

const std::vector<std::pair<int32_t, ChromaLocation>> g_pFfChromaLocationMap = {
    {0, ChromaLocation::UNSPECIFIED},
    {1, ChromaLocation::LEFT},
    {2, ChromaLocation::CENTER},
    {3, ChromaLocation::TOPLEFT},
    {4, ChromaLocation::TOP},
    {5, ChromaLocation::BOTTOMLEFT},
    {6, ChromaLocation::BOTTOM},
};

const std::vector<std::pair<int, HEVCProfile>> g_pFfHEVCProfileMap = {
    {1, HEVCProfile::HEVC_PROFILE_MAIN},
    {2, HEVCProfile::HEVC_PROFILE_MAIN_10},
    {3, HEVCProfile::HEVC_PROFILE_MAIN_STILL},
};

const std::vector<std::pair<int, HEVCLevel>> g_pFfHEVCLevelMap = {
    {30, HEVCLevel::HEVC_LEVEL_1},   {60, HEVCLevel::HEVC_LEVEL_2},  {63, HEVCLevel::HEVC_LEVEL_21},
    {90, HEVCLevel::HEVC_LEVEL_3},   {93, HEVCLevel::HEVC_LEVEL_31}, {120, HEVCLevel::HEVC_LEVEL_4},
    {123, HEVCLevel::HEVC_LEVEL_41}, {150, HEVCLevel::HEVC_LEVEL_5}, {153, HEVCLevel::HEVC_LEVEL_51},
    {156, HEVCLevel::HEVC_LEVEL_52}, {180, HEVCLevel::HEVC_LEVEL_6}, {183, HEVCLevel::HEVC_LEVEL_61},
    {186, HEVCLevel::HEVC_LEVEL_62},
};

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

ColorPrimary Converter::ConvertFFMpegToOHColorPrimaries(int32_t colorPrimaries)
{
    auto ite = std::find_if(g_pFfColorPrimariesMap.begin(), g_pFfColorPrimariesMap.end(),
                            [colorPrimaries](const auto &item) -> bool { return item.first == colorPrimaries; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfColorPrimariesMap.end(), ColorPrimary::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, colorPrimaries);
    return ite->second;
}

TransferCharacteristic Converter::ConvertFFMpegToOHColorTrans(int32_t colorTransfer)
{
    auto ite = std::find_if(g_pFfTransferCharacteristicMap.begin(), g_pFfTransferCharacteristicMap.end(),
                            [colorTransfer](const auto &item) -> bool { return item.first == colorTransfer; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfTransferCharacteristicMap.end(), TransferCharacteristic::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, colorTransfer);
    return ite->second;
}

MatrixCoefficient Converter::ConvertFFMpegToOHColorMatrix(int32_t colorSpace)
{
    auto ite = std::find_if(g_pFfMatrixCoefficientMap.begin(), g_pFfMatrixCoefficientMap.end(),
                            [colorSpace](const auto &item) -> bool { return item.first == colorSpace; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfMatrixCoefficientMap.end(), MatrixCoefficient::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, colorSpace);
    return ite->second;
}

int32_t Converter::ConvertFFMpegToOHColorRange(int32_t colorRange)
{
    auto ite = std::find_if(g_pFfColorRangeMap.begin(), g_pFfColorRangeMap.end(),
                            [colorRange](const auto &item) -> bool { return item.first == colorRange; });
    if (ite == g_pFfColorRangeMap.end()) {
        MEDIA_LOG_W("Failed: " PUBLIC_LOG_D32, colorRange);
        return 0;
    }
    return ite->second;
}

ChromaLocation Converter::ConvertFFMpegToOHChromaLocation(int32_t chromaLocation)
{
    auto ite = std::find_if(g_pFfChromaLocationMap.begin(), g_pFfChromaLocationMap.end(),
                            [chromaLocation](const auto &item) -> bool { return item.first == chromaLocation; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfChromaLocationMap.end(), ChromaLocation::UNSPECIFIED,
                             "Failed: " PUBLIC_LOG_D32, chromaLocation);
    return ite->second;
}

#ifdef USE_FF_DEMUXER
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
#endif

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

#ifdef USE_FF_DEMUXER
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
#endif

std::string_view Converter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout)
{
    auto ite = std::find_if(g_ChannelLayoutToString.begin(), g_ChannelLayoutToString.end(),
                            [&layout](const auto &item) -> bool { return item.first == layout; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_ChannelLayoutToString.end(), g_ChannelLayoutToString[0].second,
                             "Failed: " PUBLIC_LOG_D32, static_cast<int32_t>(layout));
    return ite->second;
}

#ifdef USE_FF_DEMUXER
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
#endif

void Converter::ParseColorBoxInfo(HevcParseFormat parse, Meta &format)
{
    ColorPrimary colorPrimaries = ConvertFFMpegToOHColorPrimaries(parse.colorPrimaries);
    if (colorPrimaries != ColorPrimary::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);
        format.Set<Tag::VIDEO_COLOR_RANGE>((bool)(parse.colorRange));
    } else {
        MEDIA_LOG_D("Parse colorPri from xps failed");
    }

    TransferCharacteristic colorTrans = ConvertFFMpegToOHColorTrans(parse.colorTransfer);
    if (colorTrans != TransferCharacteristic::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_TRC>(colorTrans);
    } else {
        MEDIA_LOG_D("Parse colorTrc from xps failed");
    }

    MatrixCoefficient colorMatrix = ConvertFFMpegToOHColorMatrix(parse.colorMatrixCoeff);
    if (colorMatrix != MatrixCoefficient::UNSPECIFIED) {
        format.Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrix);
    } else {
        MEDIA_LOG_D("Parse colorMatrix from xps failed");
    }

    ChromaLocation chromaLoc = ConvertFFMpegToOHChromaLocation(static_cast<int32_t>(parse.chromaLocation));
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
    constexpr size_t maxUtf8BytesPerGbkChar = 4; // max for chinese character
    size_t inLen = strGbk.length();
    if (inLen > std::numeric_limits<size_t>::max() / maxUtf8BytesPerGbkChar) {
        MEDIA_LOG_D("Output buffer size overflow");
        iconv_close(cd);
        return "";
    }
    size_t outLen = inLen * maxUtf8BytesPerGbkChar;
    char* inBuf = const_cast<char*>(strGbk.c_str());
    if (inBuf == nullptr) {
        MEDIA_LOG_D("Get in buffer failed");
        iconv_close(cd);
        return "";
    }
    char* outBuf = new (std::nothrow) char[outLen];
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
    if (data == nullptr) {
        return false;
    }
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
