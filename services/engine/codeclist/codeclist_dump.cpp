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

#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "codec_ability_singleton.h"
#include "buffer_common.h"
#include <unistd.h>
namespace OHOS {
namespace MediaAVCodec {
enum DumpIndex : uint32_t {
    INDEX_CODEC_NAME                = 0x01'01'00'00,
    INDEX_CODEC_TYPE                = 0x01'01'01'00,
    INDEX_MIME_TYPE                 = 0x01'01'02'00,
    INDEX_IS_VENDOR                 = 0x01'01'03'00,
    INDEX_MAX_INSTANCE              = 0x01'01'04'00,
    INDEX_BITRATE                   = 0x01'01'05'00,
    INDEX_CHANNELS                  = 0x01'01'06'00,
    INDEX_COMPLEXITY                = 0x01'01'07'00,
    INDEX_ALIGNMENT                 = 0x01'01'08'00,
    INDEX_WIDTH                     = 0x01'01'09'00,
    INDEX_HEIGHT                    = 0x01'01'0A'00,
    INDEX_FRAME_RATE                = 0x01'01'0B'00,
    INDEX_ENCODE_QUALITY            = 0x01'01'0C'00,
    INDEX_BLOCK_PER_FRAME           = 0x01'01'0D'00,
    INDEX_BLOCK_PER_SECOND          = 0x01'01'0E'00,
    INDEX_BLOCK_SIZE                = 0x01'01'0F'00,
    INDEX_SAMPLE_RATE               = 0x01'01'10'00,
    INDEX_PIX_FORMAT                = 0x01'01'11'00,
    INDEX_GRAPHIC_PIX_FORMAT        = 0x01'01'12'00,
    INDEX_BIT_DEPTH                 = 0x01'01'13'00,
    INDEX_PORFILES                  = 0x01'01'14'00,
    INDEX_BITRATE_MODE              = 0x01'01'15'00,
    INDEX_PROFILE_LEVELS_MAP        = 0x01'01'16'00,
    INDEX_MEASURE_FRAME_RATE        = 0x01'01'17'00,
    INDEX_SUPPORT_SWAP_WIDTH_HEIGHT = 0x01'01'18'00,
    INDEX_FEATURES_MAP              = 0x01'01'19'00,
    INDEX_RANK                      = 0x01'01'1A'00,
    INDEX_MAX_BITRATE               = 0x01'01'1B'00,
    INDEX_SQR_FACTOR                = 0x01'01'1C'00,
    INDEX_MAX_VERSION               = 0x01'01'1D'00,
    INDEX_SAMPLE_RATE_RANGES        = 0x01'01'1E'00,
};

class ProfileLevelType {
public:
    static constexpr std::string_view LEVEL_0 = "L_0";
    static constexpr std::string_view LEVEL_0B = "L_0B";
    static constexpr std::string_view LEVEL_1 = "L_1";
    static constexpr std::string_view LEVEL_10 = "L_10";
    static constexpr std::string_view LEVEL_11 = "L_11";
    static constexpr std::string_view LEVEL_12 = "L_12";
    static constexpr std::string_view LEVEL_13 = "L_13";
    static constexpr std::string_view LEVEL_1b = "L_1b";
    static constexpr std::string_view LEVEL_2 = "L_2";
    static constexpr std::string_view LEVEL_20 = "L_20";
    static constexpr std::string_view LEVEL_21 = "L_21";
    static constexpr std::string_view LEVEL_22 = "L_22";
    static constexpr std::string_view LEVEL_3 = "L_3";
    static constexpr std::string_view LEVEL_30 = "L_30";
    static constexpr std::string_view LEVEL_31 = "L_31";
    static constexpr std::string_view LEVEL_32 = "L_32";
    static constexpr std::string_view LEVEL_3B = "L_3B";
    static constexpr std::string_view LEVEL_4 = "L_4";
    static constexpr std::string_view LEVEL_40 = "L_40";
    static constexpr std::string_view LEVEL_41 = "L_41";
    static constexpr std::string_view LEVEL_42 = "L_42";
    static constexpr std::string_view LEVEL_45 = "L_45";
    static constexpr std::string_view LEVEL_4A = "L_4A";
    static constexpr std::string_view LEVEL_5 = "L_5";
    static constexpr std::string_view LEVEL_50 = "L_50";
    static constexpr std::string_view LEVEL_51 = "L_51";
    static constexpr std::string_view LEVEL_52 = "L_52";
    static constexpr std::string_view LEVEL_6 = "L_6";
    static constexpr std::string_view LEVEL_60 = "L_60";
    static constexpr std::string_view LEVEL_61 = "L_61";
    static constexpr std::string_view LEVEL_62 = "L_62";
    static constexpr std::string_view LEVEL_63 = "L_63";
    static constexpr std::string_view LEVEL_70 = "L_70";
    static constexpr std::string_view LEVEL_155 = "L_155";
    static constexpr std::string_view LEVEL_H14 = "L_H14";
    static constexpr std::string_view LEVEL_HIGH = "L_HIGH";
    static constexpr std::string_view LEVEL_HL = "L_HL";
    static constexpr std::string_view LEVEL_LL = "L_LL";
    static constexpr std::string_view LEVEL_LOW = "L_LOW";
    static constexpr std::string_view LEVEL_MEDIUM = "L_MEDIUM";
    static constexpr std::string_view LEVEL_ML = "L_ML";
    static constexpr std::string_view LEVEL_UNKNOW = "UNKNOW";
};

std::map<int32_t, std::string_view> codecTypeMap = {
    {AVCODEC_TYPE_NONE, "NONE"},
    {AVCODEC_TYPE_VIDEO_ENCODER, "VIDEO_ENCODER"},
    {AVCODEC_TYPE_VIDEO_DECODER, "VIDEO_DECODER"},
    {AVCODEC_TYPE_AUDIO_ENCODER, "AUDIO_ENCODER"},
    {AVCODEC_TYPE_AUDIO_DECODER, "AUDIO_DECODER"}
};

std::map<int32_t, std::string_view> pixFormatMap = {
    {static_cast<int32_t>(VideoPixelFormat::UNKNOWN), "UNKNOWN"},
    {static_cast<int32_t>(VideoPixelFormat::YUV420P), "YUV420P"},
    {static_cast<int32_t>(VideoPixelFormat::YUVI420), "YUVI420"},
    {static_cast<int32_t>(VideoPixelFormat::NV12), "NV12"},
    {static_cast<int32_t>(VideoPixelFormat::NV21), "NV21"},
    {static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT), "SURFACE_FORMAT"},
    {static_cast<int32_t>(VideoPixelFormat::RGBA), "RGBA"},
    {static_cast<int32_t>(VideoPixelFormat::RGBA1010102), "RGBA1010102"}
};

std::map<int32_t, std::string_view> bitrateModeMap = {
    {CBR, "CBR"},
    {VBR, "VBR"},
    {CQ, "CQ"},
    {SQR, "SQR"},
    {CBR_VIDEOCALL, "CBR_VIDEOCALL"},
    {CRF, "CRF"}
};

std::map<int32_t, std::string_view> graphicPixFormatMap = {
    {NATIVEBUFFER_PIXEL_FMT_RGBA_8888, "RGBA_8888"},
    {NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, "YCBCR_420_SP"},
    {NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, "YCRCB_420_SP"},
    {NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P, "YCBCR_420_P"},
    {NATIVEBUFFER_PIXEL_FMT_RGBA_1010102, "RGBA_1010102"},
    {NATIVEBUFFER_PIXEL_FMT_YCBCR_P010, "YCBCR_P010"},
    {NATIVEBUFFER_PIXEL_FMT_YCRCB_P010, "YCRCB_P010"}
};

std::map<std::string_view, std::map<int32_t, std::string_view>> profileMap = {
    {CodecMimeType::VIDEO_AVC, {
        {AVC_PROFILE_BASELINE, "BASELINE"},
        {AVC_PROFILE_CONSTRAINED_BASELINE, "CONSTRAINED_BASELINE"},
        {AVC_PROFILE_CONSTRAINED_HIGH, "CONSTRAINED_HIGH"},
        {AVC_PROFILE_EXTENDED, "EXTENDED"},
        {AVC_PROFILE_HIGH, "HIGH"},
        {AVC_PROFILE_HIGH_10, "HIGH_10"},
        {AVC_PROFILE_HIGH_422, "HIGH_422"},
        {AVC_PROFILE_HIGH_444, "HIGH_444"},
        {AVC_PROFILE_MAIN, "MAIN"}
    }},
    {CodecMimeType::VIDEO_HEVC, {
        {HEVC_PROFILE_MAIN, "MAIN"},
        {HEVC_PROFILE_MAIN_10, "MAIN_10"},
        {HEVC_PROFILE_MAIN_STILL, "MAIN_STILL"},
        {HEVC_PROFILE_MAIN_10_HDR10, "MAIN_10_HDR10"},
        {HEVC_PROFILE_MAIN_10_HDR10_PLUS, "MAIN_10_HDR10_PLUS"},
        {HEVC_PROFILE_UNKNOW, "UNKNOW"}
    }},
    {CodecMimeType::VIDEO_VVC, {
        {VVC_PROFILE_MAIN_10, "MAIN_10"},
        {VVC_PROFILE_MAIN_12, "MAIN_12"},
        {VVC_PROFILE_MAIN_12_INTRA, "MAIN_12_INTRA"},
        {VVC_PROFILE_MULTI_MAIN_10, "MULTI_MAIN_10"},
        {VVC_PROFILE_MAIN_10_444, "MAIN_10_444"},
        {VVC_PROFILE_MAIN_12_444, "MAIN_12_444"},
        {VVC_PROFILE_MAIN_16_444, "MAIN_16_444"},
        {VVC_PROFILE_MAIN_12_444_INTRA, "MAIN_12_444_INTRA"},
        {VVC_PROFILE_MAIN_16_444_INTRA, "MAIN_16_444_INTRA"},
        {VVC_PROFILE_MULTI_MAIN_10_444, "MULTI_MAIN_10_444"},
        {VVC_PROFILE_MAIN_10_STILL, "MAIN_10_STILL"},
        {VVC_PROFILE_MAIN_12_STILL, "MAIN_12_STILL"},
        {VVC_PROFILE_MAIN_10_444_STILL, "MAIN_10_444_STILL"},
        {VVC_PROFILE_MAIN_12_444_STILL, "MAIN_12_444_STILL"},
        {VVC_PROFILE_MAIN_16_444_STILL, "MAIN_16_444_STILL"},
        {VVC_PROFILE_UNKNOW, "UNKNOW"}
    }},
    {CodecMimeType::VIDEO_MPEG2, {
        {MPEG2_PROFILE_SIMPLE, "SIMPLE"},
        {MPEG2_PROFILE_MAIN, "MAIN"},
        {MPEG2_PROFILE_SNR, "SNR"},
        {MPEG2_PROFILE_SPATIAL, "SPATIAL"},
        {MPEG2_PROFILE_HIGH, "HIGH"},
        {MPEG2_PROFILE_422, "422"}
    }},
    {CodecMimeType::VIDEO_MPEG4, {
        {MPEG4_PROFILE_SIMPLE, "SIMPLE"},
        {MPEG4_PROFILE_SIMPLE_SCALABLE, "SIMPLE_SCALABLE"},
        {MPEG4_PROFILE_CORE, "CORE"},
        {MPEG4_PROFILE_MAIN, "MAIN"},
        {MPEG4_PROFILE_NBIT, "NBIT"},
        {MPEG4_PROFILE_HYBRID, "HYBRID"},
        {MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE, "BASIC_ANIMATED_TEXTURE"},
        {MPEG4_PROFILE_SCALABLE_TEXTURE, "SCALABLE_TEXTURE"},
        {MPEG4_PROFILE_SIMPLE_FA, "SIMPLE_FA"},
        {MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE, "ADVANCED_REAL_TIME_SIMPLE"},
        {MPEG4_PROFILE_CORE_SCALABLE, "CORE_SCALABLE"},
        {MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY, "ADVANCED_CODING_EFFICIENCY"},
        {MPEG4_PROFILE_ADVANCED_CORE, "ADVANCED_CORE"},
        {MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE, "ADVANCED_SCALABLE_TEXTURE"},
        {MPEG4_PROFILE_SIMPLE_FBA, "SIMPLE_FBA"},
        {MPEG4_PROFILE_SIMPLE_STUDIO, "SIMPLE_STUDIO"},
        {MPEG4_PROFILE_CORE_STUDIO, "CORE_STUDIO"},
        {MPEG4_PROFILE_ADVANCED_SIMPLE, "ADVANCED_SIMPLE"},
        {MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE, "FINE_GRANULARITY_SCALABLE"}
    }},
    {CodecMimeType::VIDEO_H263, {
        {H263_PROFILE_BASELINE, "BASELINE"},
        {H263_PROFILE_H320_CODING_EFFICIENCY_VERSION2_BACKWARD_COMPATIBILITY,
         "H320_CODING_EFFICIENCY_VERSION2_BACKWARD_COMPATIBILITY"},
        {H263_PROFILE_VERSION_1_BACKWARD_COMPATIBILITY, "H263_VERSION_1_BACKWARD_COMPATIBILITY"},
        {H263_PROFILE_VERSION_2_INTERACTIVE_AND_STREAMING_WIRELESS,
         "VERSION_2_INTERACTIVE_AND_STREAMING_WIRELESS"},
        {H263_PROFILE_VERSION_3_INTERACTIVE_AND_STREAMING_WIRELESS,
         "VERSION_3_INTERACTIVE_AND_STREAMING_WIRELESS"},
        {H263_PROFILE_CONVERSATIONAL_HIGH_COMPRESSION, "CONVERSATIONAL_HIGH_COMPRESSION"},
        {H263_PROFILE_CONVERSATIONAL_INTERNET, "CONVERSATIONAL_INTERNET"},
        {H263_PROFILE_CONVERSATIONAL_PLUS_INTERLACE, "CONVERSATIONAL_PLUS_INTERLACE"},
        {H263_PROFILE_HIGH_LATENCY, "HIGH_LATENCY"}
    }},
    {CodecMimeType::VIDEO_VC1, {
        {VC1_PROFILE_SIMPLE, "SIMPLE"},
        {VC1_PROFILE_MAIN, "MAIN"},
        {VC1_PROFILE_ADVANCED, "ADVANCED"}
    }},
    {CodecMimeType::VIDEO_WMV3, {
        {WMV3_PROFILE_SIMPLE, "SIMPLE"},
        {WMV3_PROFILE_MAIN, "MAIN"}
    }},
    {CodecMimeType::VIDEO_VP8, {
        {VP8_PROFILE_MAIN, "MAIN"}
    }}
};

std::map<std::string_view, std::map<int32_t, std::string_view>> profileLevelMap = {
    {CodecMimeType::VIDEO_AVC, {
        {AVC_LEVEL_1, ProfileLevelType::LEVEL_1},
        {AVC_LEVEL_1b, ProfileLevelType::LEVEL_1b},
        {AVC_LEVEL_11, ProfileLevelType::LEVEL_11},
        {AVC_LEVEL_12, ProfileLevelType::LEVEL_12},
        {AVC_LEVEL_13, ProfileLevelType::LEVEL_13},
        {AVC_LEVEL_2, ProfileLevelType::LEVEL_2},
        {AVC_LEVEL_21, ProfileLevelType::LEVEL_21},
        {AVC_LEVEL_22, ProfileLevelType::LEVEL_22},
        {AVC_LEVEL_3, ProfileLevelType::LEVEL_3},
        {AVC_LEVEL_31, ProfileLevelType::LEVEL_31},
        {AVC_LEVEL_32, ProfileLevelType::LEVEL_32},
        {AVC_LEVEL_4, ProfileLevelType::LEVEL_4},
        {AVC_LEVEL_41, ProfileLevelType::LEVEL_41},
        {AVC_LEVEL_42, ProfileLevelType::LEVEL_42},
        {AVC_LEVEL_5, ProfileLevelType::LEVEL_5},
        {AVC_LEVEL_51, ProfileLevelType::LEVEL_51},
        {AVC_LEVEL_52, ProfileLevelType::LEVEL_52},
        {AVC_LEVEL_6, ProfileLevelType::LEVEL_6},
        {AVC_LEVEL_61, ProfileLevelType::LEVEL_61},
        {AVC_LEVEL_62, ProfileLevelType::LEVEL_62}
    }},
    {CodecMimeType::VIDEO_HEVC, {
        {HEVC_LEVEL_1, ProfileLevelType::LEVEL_1},
        {HEVC_LEVEL_2, ProfileLevelType::LEVEL_2},
        {HEVC_LEVEL_21, ProfileLevelType::LEVEL_21},
        {HEVC_LEVEL_3, ProfileLevelType::LEVEL_3},
        {HEVC_LEVEL_31, ProfileLevelType::LEVEL_31},
        {HEVC_LEVEL_4, ProfileLevelType::LEVEL_4},
        {HEVC_LEVEL_41, ProfileLevelType::LEVEL_41},
        {HEVC_LEVEL_5, ProfileLevelType::LEVEL_5},
        {HEVC_LEVEL_51, ProfileLevelType::LEVEL_51},
        {HEVC_LEVEL_52, ProfileLevelType::LEVEL_52},
        {HEVC_LEVEL_6, ProfileLevelType::LEVEL_6},
        {HEVC_LEVEL_61, ProfileLevelType::LEVEL_61},
        {HEVC_LEVEL_62, ProfileLevelType::LEVEL_62},
        {HEVC_LEVEL_UNKNOW, ProfileLevelType::LEVEL_UNKNOW}
    }},
    {CodecMimeType::VIDEO_VVC, {
        {VVC_LEVEL_1, ProfileLevelType::LEVEL_1},
        {VVC_LEVEL_2, ProfileLevelType::LEVEL_2},
        {VVC_LEVEL_21, ProfileLevelType::LEVEL_21},
        {VVC_LEVEL_3, ProfileLevelType::LEVEL_3},
        {VVC_LEVEL_31, ProfileLevelType::LEVEL_31},
        {VVC_LEVEL_4, ProfileLevelType::LEVEL_4},
        {VVC_LEVEL_41, ProfileLevelType::LEVEL_41},
        {VVC_LEVEL_5, ProfileLevelType::LEVEL_5},
        {VVC_LEVEL_51, ProfileLevelType::LEVEL_51},
        {VVC_LEVEL_52, ProfileLevelType::LEVEL_52},
        {VVC_LEVEL_6, ProfileLevelType::LEVEL_6},
        {VVC_LEVEL_61, ProfileLevelType::LEVEL_61},
        {VVC_LEVEL_62, ProfileLevelType::LEVEL_62},
        {VVC_LEVEL_63, ProfileLevelType::LEVEL_63},
        {VVC_LEVEL_155, ProfileLevelType::LEVEL_155},
        {VVC_LEVEL_UNKNOW, ProfileLevelType::LEVEL_UNKNOW}
    }},
    {CodecMimeType::VIDEO_MPEG2, {
        {MPEG2_LEVEL_LL, ProfileLevelType::LEVEL_LL},
        {MPEG2_LEVEL_ML, ProfileLevelType::LEVEL_ML},
        {MPEG2_LEVEL_H14, ProfileLevelType::LEVEL_H14},
        {MPEG2_LEVEL_HL, ProfileLevelType::LEVEL_HL}
    }},
    {CodecMimeType::VIDEO_MPEG4, {
        {MPEG4_LEVEL_0, ProfileLevelType::LEVEL_0},
        {MPEG4_LEVEL_0B, ProfileLevelType::LEVEL_0B},
        {MPEG4_LEVEL_1, ProfileLevelType::LEVEL_1},
        {MPEG4_LEVEL_2, ProfileLevelType::LEVEL_2},
        {MPEG4_LEVEL_3, ProfileLevelType::LEVEL_3},
        {MPEG4_LEVEL_3B, ProfileLevelType::LEVEL_3B},
        {MPEG4_LEVEL_4, ProfileLevelType::LEVEL_4},
        {MPEG4_LEVEL_4A, ProfileLevelType::LEVEL_4A},
        {MPEG4_LEVEL_5, ProfileLevelType::LEVEL_5},
        {MPEG4_LEVEL_6, ProfileLevelType::LEVEL_6}
    }},
    {CodecMimeType::VIDEO_H263, {
        {H263_LEVEL_10, ProfileLevelType::LEVEL_10},
        {H263_LEVEL_20, ProfileLevelType::LEVEL_20},
        {H263_LEVEL_30, ProfileLevelType::LEVEL_30},
        {H263_LEVEL_40, ProfileLevelType::LEVEL_40},
        {H263_LEVEL_45, ProfileLevelType::LEVEL_45},
        {H263_LEVEL_50, ProfileLevelType::LEVEL_50},
        {H263_LEVEL_60, ProfileLevelType::LEVEL_60},
        {H263_LEVEL_70, ProfileLevelType::LEVEL_70}
    }},
    {CodecMimeType::VIDEO_VC1, {
        {VC1_LEVEL_L0, ProfileLevelType::LEVEL_0},
        {VC1_LEVEL_L1, ProfileLevelType::LEVEL_1},
        {VC1_LEVEL_L2, ProfileLevelType::LEVEL_2},
        {VC1_LEVEL_L3, ProfileLevelType::LEVEL_3},
        {VC1_LEVEL_L4, ProfileLevelType::LEVEL_4},
        {VC1_LEVEL_LOW, ProfileLevelType::LEVEL_LOW},
        {VC1_LEVEL_MEDIUM, ProfileLevelType::LEVEL_MEDIUM},
        {VC1_LEVEL_HIGH, ProfileLevelType::LEVEL_HIGH}
    }},
    {CodecMimeType::VIDEO_WMV3, {
        {WMV3_LEVEL_LOW, ProfileLevelType::LEVEL_LOW},
        {WMV3_LEVEL_MEDIUM, ProfileLevelType::LEVEL_MEDIUM},
        {WMV3_LEVEL_HIGH, ProfileLevelType::LEVEL_HIGH}
    }}
};

std::string FormatRange(const Range& range)
{
    if (range.minVal == 0 && range.maxVal == 0) {
        return "";
    }
    return std::to_string(range.minVal) + "-" + std::to_string(range.maxVal);
}

std::string FormatImgSize(const ImgSize& imgSize)
{
    if (imgSize.width == 0 && imgSize.height == 0) {
        return "";
    }
    return std::to_string(imgSize.width) + "*" + std::to_string(imgSize.height);
}

std::string ToString(int32_t index, const std::map<int32_t, std::string_view>& map)
{
    auto iter = map.find(index);
    return iter != map.end() ? iter->second.data() : std::to_string(index);
}

std::string FormatVector(const std::vector<int32_t>& vec, const std::map<int32_t, std::string_view>& map)
{
    if (vec.size() == 0) {
        return "";
    }
    std::string retStr = "<";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            retStr += ", ";
        }
        retStr += ToString(vec[i], map);
    }
    retStr += ">";
    return retStr;
}

std::string FormatMeasuredFrameRateMap(const std::map<ImgSize, Range>& map)
{
    std::string retStr;
    bool isFirstPair = true;
    for (auto iter : map) {
        if (!isFirstPair) {
            retStr += ", ";
        }
        retStr += "(" + FormatImgSize(iter.first) + ", " + FormatRange(iter.second) + ")";
        isFirstPair = false;
    }
    return retStr;
}

std::string FormatProfileLevelsMap(std::string_view mimeType, std::string spaceStr,
                                   const std::map<int32_t, std::vector<int32_t>>& map)
{
    std::string retStr;
    bool isFirstPair = true;
    bool hasKeyMapping = profileMap.find(mimeType) != profileMap.end();
    bool hasValueMapping = profileLevelMap.find(mimeType) != profileLevelMap.end();
    for (auto iter : map) {
        if (!isFirstPair) {
            retStr += ",\n" + spaceStr;
        }
        retStr += "(" ;
        if (hasKeyMapping) {
            retStr += ToString(iter.first, profileMap[mimeType]);
        } else {
            retStr += std::to_string(iter.first);
        }
        retStr += ", " ;
        if (hasValueMapping) {
            retStr += FormatVector(iter.second, profileLevelMap[mimeType]) ;
        } else {
            std::map<int32_t, std::string_view> emptyMap;
            retStr += FormatVector(iter.second, emptyMap) ;
        }
        retStr += ")";
        isFirstPair = false;
    }
    return retStr;
}

std::string FormatSampleRateRange(const std::vector<Range>& vec)
{
    if (vec.size() == 0) {
        return "";
    }
    std::string retStr = "<";
    bool isFirstPair = true;
    for (auto iter : vec) {
        if (!isFirstPair) {
            retStr += ",";
        }
        retStr += FormatRange(iter);
        isFirstPair = false;
    }
    retStr += ">";
    return retStr;
}

void AddInfo(AVCodecDumpControler& dumpCtrl, const uint32_t dumpIdex,
             const std::string& name, const std::string& value)
{
    if (!value.empty()) {
        dumpCtrl.AddInfo(dumpIdex, name, value);
    }
}

bool StartsWith(const std::string& self, const std::string& prefix)
{
    if (self.size() < prefix.size()){
        return false;
    }
    return self.compare(0, prefix.size(), prefix) == 0;
}

void AddCapabilityDataDump(AVCodecDumpControler& dumpCtrl, CapabilityData& capability)
{
    dumpCtrl.AddInfo(INDEX_CODEC_NAME, capability.codecName, "");
    AddInfo(dumpCtrl, INDEX_CODEC_TYPE, "codecType", ToString(capability.codecType, codecTypeMap));
    AddInfo(dumpCtrl, INDEX_MIME_TYPE, "mimeType", capability.mimeType);
    AddInfo(dumpCtrl, INDEX_IS_VENDOR, "isVendor", capability.isVendor ? "True" : "False");
    AddInfo(dumpCtrl, INDEX_BITRATE, "bitrate", FormatRange(capability.bitrate));
    AddInfo(dumpCtrl, INDEX_CHANNELS, "channels", FormatRange(capability.channels));
    AddInfo(dumpCtrl, INDEX_COMPLEXITY, "complexity", FormatRange(capability.complexity));
    AddInfo(dumpCtrl, INDEX_ALIGNMENT, "alignment", FormatImgSize(capability.alignment));
    AddInfo(dumpCtrl, INDEX_WIDTH, "width", FormatRange(capability.width));
    AddInfo(dumpCtrl, INDEX_HEIGHT, "height", FormatRange(capability.height));
    AddInfo(dumpCtrl, INDEX_FRAME_RATE, "frameRate", FormatRange(capability.frameRate));
    AddInfo(dumpCtrl, INDEX_ENCODE_QUALITY, "encodeQuality", FormatRange(capability.encodeQuality));
    AddInfo(dumpCtrl, INDEX_BLOCK_PER_FRAME, "blockPerFrame", FormatRange(capability.blockPerFrame));
    AddInfo(dumpCtrl, INDEX_BLOCK_PER_SECOND, "blockPerSecond", FormatRange(capability.blockPerSecond));
    AddInfo(dumpCtrl, INDEX_BLOCK_SIZE, "blockSize", FormatImgSize(capability.blockSize));
    std::map<int32_t, std::string_view> emptyMap;
    AddInfo(dumpCtrl, INDEX_SAMPLE_RATE, "sampleRate", FormatVector(capability.sampleRate, emptyMap));
    AddInfo(dumpCtrl, INDEX_PIX_FORMAT, "pixFormat", FormatVector(capability.pixFormat, pixFormatMap));
    AddInfo(dumpCtrl, INDEX_GRAPHIC_PIX_FORMAT, "graphicPixFormat",
            FormatVector(capability.graphicPixFormat, graphicPixFormatMap));
    AddInfo(dumpCtrl, INDEX_BIT_DEPTH, "bitDepth", FormatVector(capability.bitDepth, emptyMap));
    AddInfo(dumpCtrl, INDEX_BITRATE_MODE, "bitrateMode", FormatVector(capability.bitrateMode, bitrateModeMap));
    AddInfo(dumpCtrl, INDEX_MEASURE_FRAME_RATE, "measuredFrameRate",
            FormatMeasuredFrameRateMap(capability.measuredFrameRate));
    if (capability.codecType == AVCODEC_TYPE_VIDEO_ENCODER || capability.codecType == AVCODEC_TYPE_VIDEO_DECODER) {
        AddInfo(dumpCtrl, INDEX_MAX_INSTANCE, "maxInstance", std::to_string(capability.maxInstance));
        std::string suportSwap = capability.supportSwapWidthHeight ? "True" : "False";
        AddInfo(dumpCtrl, INDEX_SUPPORT_SWAP_WIDTH_HEIGHT, "supportSwapWidthHeight", suportSwap);
    }
    std::string spaceStr = std::string(dumpCtrl.GetSpaceLength(INDEX_PROFILE_LEVELS_MAP), ' ');
    AddInfo(dumpCtrl, INDEX_PROFILE_LEVELS_MAP, "profileLevelsMap",
            FormatProfileLevelsMap(capability.mimeType, spaceStr, capability.profileLevelsMap));
    AddInfo(dumpCtrl, INDEX_RANK, "rank", capability.rank ? std::to_string(capability.rank) : "");
    AddInfo(dumpCtrl, INDEX_MAX_BITRATE, "maxBitrate", FormatRange(capability.maxBitrate));
    AddInfo(dumpCtrl, INDEX_SQR_FACTOR, "sqrFactor", FormatRange(capability.sqrFactor));
    std::string maxVersionStr = capability.maxVersion ? std::to_string(capability.maxVersion) : "";
    AddInfo(dumpCtrl, INDEX_MAX_VERSION, "maxVersion", maxVersionStr);
    AddInfo(dumpCtrl, INDEX_SAMPLE_RATE_RANGES, "sampleRateRanges",
            FormatSampleRateRange(capability.sampleRateRanges));
}

int32_t CodecAbilitySingleton::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (fd < 0) {
        return AVCS_ERR_OK;
    }
    std::string typeStr = "";
    if (args.size() > 1) {
        if (args[1] == u"video") {
            typeStr = "video";
        } else if (args[1] == u"audio") {
            typeStr = "audio";
        }
    }
    constexpr std::string_view capabilityStr = "[Capability]\n";
    write(fd, capabilityStr.data(), capabilityStr.size());
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto iter : capabilityDataArray_) {
        if (!StartsWith(iter.mimeType, typeStr)) {
            continue;
        }
        AVCodecDumpControler dumpCtrl;
        AddCapabilityDataDump(dumpCtrl, iter);
        std::string dumpString;
        dumpCtrl.GetDumpString(dumpString);
        dumpString += "\n";
        write(fd, dumpString.c_str(), dumpString.size());
    }
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS