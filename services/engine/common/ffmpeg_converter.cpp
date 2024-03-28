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
#include <vector>
#include "avcodec_log.h"
#include "ffmpeg_converter.h"
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "FFmpegConverter"};
    constexpr int US_PER_SECOND = 1000000;
}
namespace OHOS {
namespace MediaAVCodec {
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
    {AudioChannelLayout::UNKNOWN_CHANNEL_LAYOUT, "UNKNOW"},
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
    {AudioChannelLayout::HOA_FIRST, "HOA_FIRST"},
    {AudioChannelLayout::HOA_SECOND, "HOA_SECOND"},
    {AudioChannelLayout::HOA_THIRD, "HOA_THIRD"},
};

const std::vector<std::pair<AVColorPrimaries, ColorPrimary>> g_pFfColorPrimariesMap = {
    {AVColorPrimaries::AVCOL_PRI_BT709, ColorPrimary::COLOR_PRIMARY_BT709},
    {AVColorPrimaries::AVCOL_PRI_UNSPECIFIED, ColorPrimary::COLOR_PRIMARY_UNSPECIFIED},
    {AVColorPrimaries::AVCOL_PRI_BT470M, ColorPrimary::COLOR_PRIMARY_BT470_M},
    {AVColorPrimaries::AVCOL_PRI_BT470BG, ColorPrimary::COLOR_PRIMARY_BT601_625},
    {AVColorPrimaries::AVCOL_PRI_SMPTE170M, ColorPrimary::COLOR_PRIMARY_BT601_525},
    {AVColorPrimaries::AVCOL_PRI_SMPTE240M, ColorPrimary::COLOR_PRIMARY_SMPTE_ST240},
    {AVColorPrimaries::AVCOL_PRI_FILM, ColorPrimary::COLOR_PRIMARY_GENERIC_FILM},
    {AVColorPrimaries::AVCOL_PRI_BT2020, ColorPrimary::COLOR_PRIMARY_BT2020},
    {AVColorPrimaries::AVCOL_PRI_SMPTE428, ColorPrimary::COLOR_PRIMARY_SMPTE_ST428},
    {AVColorPrimaries::AVCOL_PRI_SMPTEST428_1, ColorPrimary::COLOR_PRIMARY_SMPTE_ST428},
    {AVColorPrimaries::AVCOL_PRI_SMPTE431, ColorPrimary::COLOR_PRIMARY_P3DCI},
    {AVColorPrimaries::AVCOL_PRI_SMPTE432, ColorPrimary::COLOR_PRIMARY_P3D65},
};

const std::vector<std::pair<AVColorTransferCharacteristic, TransferCharacteristic>> g_pFfTransferCharacteristicMap = {
    {AVColorTransferCharacteristic::AVCOL_TRC_BT709, TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709},
    {AVColorTransferCharacteristic::AVCOL_TRC_UNSPECIFIED, TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED},
    {AVColorTransferCharacteristic::AVCOL_TRC_GAMMA22, TransferCharacteristic::TRANSFER_CHARACTERISTIC_GAMMA_2_2},
    {AVColorTransferCharacteristic::AVCOL_TRC_GAMMA28, TransferCharacteristic::TRANSFER_CHARACTERISTIC_GAMMA_2_8},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE170M, TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT601},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE240M, TransferCharacteristic::TRANSFER_CHARACTERISTIC_SMPTE_ST240},
    {AVColorTransferCharacteristic::AVCOL_TRC_LINEAR, TransferCharacteristic::TRANSFER_CHARACTERISTIC_LINEAR},
    {AVColorTransferCharacteristic::AVCOL_TRC_LOG, TransferCharacteristic::TRANSFER_CHARACTERISTIC_LOG},
    {AVColorTransferCharacteristic::AVCOL_TRC_LOG_SQRT, TransferCharacteristic::TRANSFER_CHARACTERISTIC_LOG_SQRT},
    {AVColorTransferCharacteristic::AVCOL_TRC_IEC61966_2_4,
        TransferCharacteristic::TRANSFER_CHARACTERISTIC_IEC_61966_2_4},
    {AVColorTransferCharacteristic::AVCOL_TRC_BT1361_ECG, TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT1361},
    {AVColorTransferCharacteristic::AVCOL_TRC_IEC61966_2_1,
        TransferCharacteristic::TRANSFER_CHARACTERISTIC_IEC_61966_2_1},
    {AVColorTransferCharacteristic::AVCOL_TRC_BT2020_10, TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT2020_10BIT},
    {AVColorTransferCharacteristic::AVCOL_TRC_BT2020_12, TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT2020_12BIT},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE2084, TransferCharacteristic::TRANSFER_CHARACTERISTIC_PQ},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTEST2084, TransferCharacteristic::TRANSFER_CHARACTERISTIC_PQ},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTE428, TransferCharacteristic::TRANSFER_CHARACTERISTIC_SMPTE_ST428},
    {AVColorTransferCharacteristic::AVCOL_TRC_SMPTEST428_1,
        TransferCharacteristic::TRANSFER_CHARACTERISTIC_SMPTE_ST428},
    {AVColorTransferCharacteristic::AVCOL_TRC_ARIB_STD_B67, TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG},
};

const std::vector<std::pair<AVColorSpace, MatrixCoefficient>> g_pFfMatrixCoefficientMap = {
    {AVColorSpace::AVCOL_SPC_RGB, MatrixCoefficient::MATRIX_COEFFICIENT_IDENTITY},
    {AVColorSpace::AVCOL_SPC_BT709, MatrixCoefficient::MATRIX_COEFFICIENT_BT709},
    {AVColorSpace::AVCOL_SPC_UNSPECIFIED, MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED},
    {AVColorSpace::AVCOL_SPC_FCC, MatrixCoefficient::MATRIX_COEFFICIENT_FCC},
    {AVColorSpace::AVCOL_SPC_BT470BG, MatrixCoefficient::MATRIX_COEFFICIENT_BT601_625},
    {AVColorSpace::AVCOL_SPC_SMPTE170M, MatrixCoefficient::MATRIX_COEFFICIENT_BT601_525},
    {AVColorSpace::AVCOL_SPC_SMPTE240M, MatrixCoefficient::MATRIX_COEFFICIENT_SMPTE_ST240},
    {AVColorSpace::AVCOL_SPC_YCGCO, MatrixCoefficient::MATRIX_COEFFICIENT_YCGCO},
    {AVColorSpace::AVCOL_SPC_YCOCG, MatrixCoefficient::MATRIX_COEFFICIENT_YCGCO},
    {AVColorSpace::AVCOL_SPC_BT2020_NCL, MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL},
    {AVColorSpace::AVCOL_SPC_BT2020_CL, MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_CL },
    {AVColorSpace::AVCOL_SPC_SMPTE2085, MatrixCoefficient::MATRIX_COEFFICIENT_SMPTE_ST2085 },
    {AVColorSpace::AVCOL_SPC_CHROMA_DERIVED_NCL, MatrixCoefficient::MATRIX_COEFFICIENT_CHROMATICITY_NCL },
    {AVColorSpace::AVCOL_SPC_CHROMA_DERIVED_CL, MatrixCoefficient::MATRIX_COEFFICIENT_CHROMATICITY_CL },
    {AVColorSpace::AVCOL_SPC_ICTCP, MatrixCoefficient::MATRIX_COEFFICIENT_ICTCP },
};

const std::vector<std::pair<AVColorRange, int>> g_pFfColorRangeMap = {
    {AVColorRange::AVCOL_RANGE_MPEG, 0},
    {AVColorRange::AVCOL_RANGE_JPEG, 1},
};

const std::vector<std::pair<AVChromaLocation, ChromaLocation>> g_pFfChromaLocationMap = {
    {AVChromaLocation::AVCHROMA_LOC_UNSPECIFIED, ChromaLocation::CHROMA_LOC_UNSPECIFIED},
    {AVChromaLocation::AVCHROMA_LOC_LEFT, ChromaLocation::CHROMA_LOC_LEFT},
    {AVChromaLocation::AVCHROMA_LOC_CENTER, ChromaLocation::CHROMA_LOC_CENTER},
    {AVChromaLocation::AVCHROMA_LOC_TOPLEFT, ChromaLocation::CHROMA_LOC_TOPLEFT},
    {AVChromaLocation::AVCHROMA_LOC_TOP, ChromaLocation::CHROMA_LOC_TOP},
    {AVChromaLocation::AVCHROMA_LOC_BOTTOMLEFT, ChromaLocation::CHROMA_LOC_BOTTOMLEFT},
    {AVChromaLocation::AVCHROMA_LOC_BOTTOM, ChromaLocation::CHROMA_LOC_BOTTOM},
};

const std::vector<std::pair<int, HEVCProfile>> g_pFfHEVCProfileMap = {
    {FF_PROFILE_HEVC_MAIN, HEVCProfile::HEVC_PROFILE_MAIN},
    {FF_PROFILE_HEVC_MAIN_10, HEVCProfile::HEVC_PROFILE_MAIN_10},
    {FF_PROFILE_HEVC_MAIN_STILL_PICTURE, HEVCProfile::HEVC_PROFILE_MAIN_STILL},
};

const std::vector<std::pair<int, HEVCLevel>> g_pFfHEVCLevelMap = {
    {30, HEVCLevel::HEVC_LEVEL_1},
    {60, HEVCLevel::HEVC_LEVEL_2},
    {63, HEVCLevel::HEVC_LEVEL_21},
    {90, HEVCLevel::HEVC_LEVEL_3},
    {93, HEVCLevel::HEVC_LEVEL_31},
    {120, HEVCLevel::HEVC_LEVEL_4},
    {123, HEVCLevel::HEVC_LEVEL_41},
    {150, HEVCLevel::HEVC_LEVEL_5},
    {153, HEVCLevel::HEVC_LEVEL_51},
    {156, HEVCLevel::HEVC_LEVEL_52},
    {180, HEVCLevel::HEVC_LEVEL_6},
    {183, HEVCLevel::HEVC_LEVEL_61},
    {186, HEVCLevel::HEVC_LEVEL_62},
};

HEVCLevel FFMpegConverter::ConvertFFMpegToOHHEVCLevel(int ffHEVCLevel)
{
    auto ite = std::find_if(g_pFfHEVCLevelMap.begin(), g_pFfHEVCLevelMap.end(),
                            [&ffHEVCLevel](const auto &item) -> bool { return item.first == ffHEVCLevel; });
    if (ite == g_pFfHEVCLevelMap.end()) {
        AVCODEC_LOGW("Convert hevc level failed: %{public}d", ffHEVCLevel);
        return HEVCLevel::HEVC_LEVEL_UNKNOW;
    }
    return ite->second;
}

HEVCProfile FFMpegConverter::ConvertFFMpegToOHHEVCProfile(int ffHEVCProfile)
{
    auto ite = std::find_if(g_pFfHEVCProfileMap.begin(), g_pFfHEVCProfileMap.end(),
                            [&ffHEVCProfile](const auto &item) -> bool { return item.first == ffHEVCProfile; });
    if (ite == g_pFfHEVCProfileMap.end()) {
        AVCODEC_LOGW("Convert hevc profile failed: %{public}d", ffHEVCProfile);
        return HEVCProfile::HEVC_PROFILE_UNKNOW;
    }
    return ite->second;
}

ColorPrimary FFMpegConverter::ConvertFFMpegToOHColorPrimaries(AVColorPrimaries ffColorPrimaries)
{
    auto ite = std::find_if(g_pFfColorPrimariesMap.begin(), g_pFfColorPrimariesMap.end(),
                            [&ffColorPrimaries](const auto &item) -> bool { return item.first == ffColorPrimaries; });
    if (ite == g_pFfColorPrimariesMap.end()) {
        AVCODEC_LOGW("Convert color primaries failed: %{public}d", static_cast<int32_t>(ffColorPrimaries));
        return ColorPrimary::COLOR_PRIMARY_UNSPECIFIED;
    }
    return ite->second;
}

TransferCharacteristic FFMpegConverter::ConvertFFMpegToOHColorTrans(AVColorTransferCharacteristic ffColorTrans)
{
    auto ite = std::find_if(g_pFfTransferCharacteristicMap.begin(), g_pFfTransferCharacteristicMap.end(),
                            [&ffColorTrans](const auto &item) -> bool { return item.first == ffColorTrans; });
    if (ite == g_pFfTransferCharacteristicMap.end()) {
        AVCODEC_LOGW("Convert color trans failed: %{public}d", static_cast<int32_t>(ffColorTrans));
        return TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED;
    }
    return ite->second;
}

MatrixCoefficient FFMpegConverter::ConvertFFMpegToOHColorMatrix(AVColorSpace ffColorSpace)
{
    auto ite = std::find_if(g_pFfMatrixCoefficientMap.begin(), g_pFfMatrixCoefficientMap.end(),
                            [&ffColorSpace](const auto &item) -> bool { return item.first == ffColorSpace; });
    if (ite == g_pFfMatrixCoefficientMap.end()) {
        AVCODEC_LOGW("Convert color matrix failed: %{public}d", static_cast<int32_t>(ffColorSpace));
        return MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED;
    }
    return ite->second;
}

int FFMpegConverter::ConvertFFMpegToOHColorRange(AVColorRange ffColorRange)
{
    auto ite = std::find_if(g_pFfColorRangeMap.begin(), g_pFfColorRangeMap.end(),
                            [&ffColorRange](const auto &item) -> bool { return item.first == ffColorRange; });
    if (ite == g_pFfColorRangeMap.end()) {
        AVCODEC_LOGW("Convert color range failed: %{public}d", static_cast<int32_t>(ffColorRange));
        return 0;
    }
    return ite->second;
}

ChromaLocation FFMpegConverter::ConvertFFMpegToOHChromaLocation(AVChromaLocation ffChromaLocation)
{
    auto ite = std::find_if(g_pFfChromaLocationMap.begin(), g_pFfChromaLocationMap.end(),
                            [&ffChromaLocation](const auto &item) -> bool { return item.first == ffChromaLocation; });
    if (ite == g_pFfChromaLocationMap.end()) {
        AVCODEC_LOGW("Convert chroma location failed: %{public}d", static_cast<int32_t>(ffChromaLocation));
        return ChromaLocation::CHROMA_LOC_UNSPECIFIED;
    }
    return ite->second;
}

AudioSampleFormat FFMpegConverter::ConvertFFMpegAVCodecIdToOHAudioFormat(AVCodecID codecId)
{
    auto ite = std::find_if(g_pFfCodeIDToSampleFmtMap.begin(), g_pFfCodeIDToSampleFmtMap.end(),
                            [&codecId](const auto &item) -> bool { return item.first == codecId; });
    if (ite == g_pFfCodeIDToSampleFmtMap.end()) {
        AVCODEC_LOGW("Convert codec id failed: %{public}d", static_cast<int32_t>(codecId));
        return AudioSampleFormat::INVALID_WIDTH;
    }
    return ite->second;
}

AudioSampleFormat FFMpegConverter::ConvertFFMpegToOHAudioFormat(AVSampleFormat ffSampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&ffSampleFormat](const auto &item) -> bool { return item.first == ffSampleFormat; });
    if (ite == g_pFfSampleFmtMap.end()) {
        AVCODEC_LOGW("Convert sample format failed: %{public}d", static_cast<int32_t>(ffSampleFormat));
        return AudioSampleFormat::INVALID_WIDTH;
    }
    return ite->second;
}

AVSampleFormat FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat sampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&sampleFormat](const auto &item) -> bool { return item.second == sampleFormat; });
    if (ite == g_pFfSampleFmtMap.end()) {
        AVCODEC_LOGW("Convert sample format failed: %{public}d", static_cast<int32_t>(sampleFormat));
        return AVSampleFormat::AV_SAMPLE_FMT_NONE;
    }
    return ite->first;
}

AudioChannelLayout FFMpegConverter::ConvertFFToOHAudioChannelLayout(uint64_t ffChannelLayout)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&ffChannelLayout](const auto &item) -> bool { return item.second == ffChannelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        AVCODEC_LOGW("Convert channel layout failed: %{public}" PRIu64, ffChannelLayout);
        return AudioChannelLayout::MONO;
    }
    return ite->first;
}

AudioChannelLayout FFMpegConverter::GetDefaultChannelLayout(int channels)
{
    AudioChannelLayout ret;
    switch (channels) {
        case 2: { // 2: STEREO
            ret = AudioChannelLayout::STEREO;
            break;
        }
        case 4: { // 4: CH_4POINT0
            ret = AudioChannelLayout::CH_4POINT0;
            break;
        }
        case 6: { // 6: CH_5POINT1
            ret = AudioChannelLayout::CH_5POINT1;
            break;
        }
        case 8: { // 8: CH_5POINT1POINT2
            ret = AudioChannelLayout::CH_5POINT1POINT2;
            break;
        }
        case 10: { // 10: CH_7POINT1POINT2 or CH_5POINT1POINT4 ?
            ret = AudioChannelLayout::CH_7POINT1POINT2;
            break;
        }
        case 12: { // 12: CH_7POINT1POINT4
            ret = AudioChannelLayout::CH_7POINT1POINT4;
            break;
        }
        case 14: { // 14: CH_9POINT1POINT4
            ret = AudioChannelLayout::CH_9POINT1POINT4;
            break;
        }
        case 16: { // 16: CH_9POINT1POINT6
            ret = AudioChannelLayout::CH_9POINT1POINT6;
            break;
        }
        case 24: { // 24: CH_22POINT2
            ret = AudioChannelLayout::CH_22POINT2;
            break;
        }
        default: {
            ret = AudioChannelLayout::MONO;
            break;
        }
    }
    AVCODEC_LOGW("Get default channel layout: %{public}s", ConvertOHAudioChannelLayoutToString(ret).data());
    return ret;
}

AudioChannelLayout FFMpegConverter::ConvertFFToOHAudioChannelLayoutV2(uint64_t ffChannelLayout, int channels)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&ffChannelLayout](const auto &item) -> bool { return item.second == ffChannelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        AVCODEC_LOGW("Convert channel layout failed: %{public}" PRIu64, ffChannelLayout);
        return GetDefaultChannelLayout(channels);
    }
    return ite->first;
}

uint64_t FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout channelLayout)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&channelLayout](const auto &item) -> bool { return item.first == channelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        AVCODEC_LOGW("Convert channel layout failed: %{public}d", static_cast<int32_t>(channelLayout));
        return AV_CH_LAYOUT_NATIVE;
    }
    return ite->second;
}

std::string_view FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout)
{
    auto ite = std::find_if(g_ChannelLayoutToString.begin(), g_ChannelLayoutToString.end(),
                            [&layout](const auto &item) -> bool { return item.first == layout; });
    if (ite == g_ChannelLayoutToString.end()) {
        AVCODEC_LOGW("Convert channel layout failed: %{public}d", static_cast<int32_t>(layout));
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
} // namespace MediaAVCodec
} // namespace OHOS