/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "fcodec.h"
#include "avcodec_codec_name.h"
#include "fcodec_surport_codec.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr int32_t VIDEO_MIN_SIZE = 2;
constexpr int32_t VIDEO_ALIGNMENT_SIZE = 2;
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 4096;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 4096;
constexpr int32_t VIDEO_MAX_WIDTH_H263_SIZE = 2048;
constexpr int32_t VIDEO_MAX_HEIGHT_H263_SIZE = 1152;
constexpr int32_t VIDEO_MIN_WIDTH_H263_SIZE = 20;
constexpr int32_t VIDEO_MIN_HEIGHT_H263_SIZE = 20;
constexpr int32_t VIDEO_INSTANCE_SIZE = 64;
constexpr int32_t VIDEO_BITRATE_MAX_SIZE = 300000000;
constexpr int32_t VIDEO_FRAMERATE_MAX_SIZE = 120;
constexpr int32_t VIDEO_FRAMERATE_DEFAULT_SIZE = 60;
constexpr int32_t VIDEO_BLOCKPERFRAME_SIZE = 139264;
constexpr int32_t VIDEO_BLOCKPERSEC_SIZE = 983040;
#ifdef SUPPORT_CODEC_VC1
constexpr int32_t VC1_ALIGNMENT_SIZE = 2;
constexpr int32_t VC1_MAX_WIDTH_SIZE = 2048;
constexpr int32_t VC1_MAX_HEIGHT_SIZE = 2048;
constexpr int32_t VC1_BITRATE_MAX_SIZE = 135000000;
constexpr int32_t VC1_BLOCKPERFRAME_SIZE = 16384;
constexpr int32_t VC1_BLOCKPERSEC_SIZE = 491520;
#endif
constexpr int32_t MSVIDEO1_MIN_WIDTH_SIZE = 4;
constexpr int32_t MSVIDEO1_MIN_HEIGHT_SIZE = 4;
constexpr int32_t MSVIDEO1_BLOCKPERSEC_SIZE = 3932160;
constexpr int32_t WMV3_ALIGNMENT_SIZE = 2;
constexpr int32_t WMV3_MAX_FRAMERATE_SIZE = 30;
constexpr int32_t WMV3_MIN_WIDTH_SIZE = 16;
constexpr int32_t WMV3_MAX_WIDTH_SIZE = 1920;
constexpr int32_t WMV3_MIN_HEIGHT_SIZE = 16;
constexpr int32_t WMV3_MAX_HEIGHT_SIZE = 1080;
constexpr int32_t WMV3_BITRATE_MAX_SIZE = 20000000;
constexpr int32_t WMV3_MAX_BLOCKPERFRAME_SIZE = 8192;
constexpr int32_t WMV3_MAX_BLOCKPERSEC_SIZE = 245760;
} // namespace
using namespace OHOS::Media;

void GetMpeg2CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.profiles = {static_cast<int32_t>(MPEG2_PROFILE_422), static_cast<int32_t>(MPEG2_PROFILE_HIGH),
                            static_cast<int32_t>(MPEG2_PROFILE_MAIN), static_cast<int32_t>(MPEG2_PROFILE_SNR),
                            static_cast<int32_t>(MPEG2_PROFILE_SIMPLE), static_cast<int32_t>(MPEG2_PROFILE_SPATIAL)};
        std::vector<int32_t> levels_sp;
        std::vector<int32_t> levels_mp;
        std::vector<int32_t> levels_snr;
        std::vector<int32_t> levels_422p;
        for (int32_t j = 0; j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_ML); ++j) {
            levels_sp.emplace_back(j);
        }
        for (int32_t j = 0; j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_HL); ++j) {
            levels_mp.emplace_back(j);
        }
        for (int32_t j = 0; j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_H14); ++j) {
            levels_snr.emplace_back(j);
        }
        for (int32_t j = static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_ML);
                j <= static_cast<int32_t>(MPEG2Level::MPEG2_LEVEL_HL); ++j) {
            levels_422p.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_SIMPLE), levels_sp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_MAIN), levels_mp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_SNR), levels_snr));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_SPATIAL), levels_mp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_HIGH), levels_mp));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(MPEG2_PROFILE_422), levels_422p));
    }
}

void SetMpeg4Profiles(CapabilityData& capsData)
{
    capsData.profiles = {
        static_cast<int32_t>(MPEG4_PROFILE_SIMPLE),
        static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_SCALABLE),
        static_cast<int32_t>(MPEG4_PROFILE_CORE),
        static_cast<int32_t>(MPEG4_PROFILE_MAIN),
        static_cast<int32_t>(MPEG4_PROFILE_NBIT),
        static_cast<int32_t>(MPEG4_PROFILE_HYBRID),
        static_cast<int32_t>(MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE),
        static_cast<int32_t>(MPEG4_PROFILE_SCALABLE_TEXTURE),
        static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_FA),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE),
        static_cast<int32_t>(MPEG4_PROFILE_CORE_SCALABLE),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CORE),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE),
        static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SIMPLE),
    };
}

void SetMpeg4LevelsProfileGroup1(CapabilityData& capsData)
{
    capsData.profileLevelsMap = {
        {static_cast<int32_t>(MPEG4_PROFILE_SIMPLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0B),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4A),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_5), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_6)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_SCALABLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_CORE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_MAIN), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_NBIT), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_HYBRID), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_SCALABLE_TEXTURE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1)
        }}
    };
}

void SetMpeg4LevelsProfileGroup2(CapabilityData& capsData)
{
    capsData.profileLevelsMap.insert({
        {static_cast<int32_t>(MPEG4_PROFILE_SIMPLE_FA), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_CORE_SCALABLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_CORE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3)
        }},
        {static_cast<int32_t>(MPEG4_PROFILE_ADVANCED_SIMPLE), {
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_0), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_1),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_2), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_3B), static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_4),
            static_cast<int32_t>(MPEG4Level::MPEG4_LEVEL_5)
        }}
    });
}

void GetMpeg4esCapProf(std::vector<CapabilityData>& capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        SetMpeg4Profiles(capsData);
        SetMpeg4LevelsProfileGroup1(capsData);
        SetMpeg4LevelsProfileGroup2(capsData);
    }
}

void GetH263CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.width.minVal = VIDEO_MIN_WIDTH_H263_SIZE;
        capsData.height.minVal = VIDEO_MIN_HEIGHT_H263_SIZE;
        capsData.width.maxVal = VIDEO_MAX_WIDTH_H263_SIZE;
        capsData.height.maxVal = VIDEO_MAX_HEIGHT_H263_SIZE;
        capsData.profiles = {static_cast<int32_t>(H263_PROFILE_BASELINE),
                             static_cast<int32_t>(H263_PROFILE_VERSION_1_BACKWARD_COMPATIBILITY)};
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(H263Level::H263_LEVEL_70); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(H263_PROFILE_BASELINE), levels));
        capsData.profileLevelsMap.insert(
            std::make_pair(static_cast<int32_t>(H263_PROFILE_VERSION_1_BACKWARD_COMPATIBILITY), levels));
    }
    return;
}

void GetAvcCapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.profiles = {static_cast<int32_t>(AVC_PROFILE_BASELINE), static_cast<int32_t>(AVC_PROFILE_MAIN),
                                    static_cast<int32_t>(AVC_PROFILE_HIGH)};
        std::vector<int32_t> levels;
        for (int32_t j = 0; j <= static_cast<int32_t>(AVCLevel::AVC_LEVEL_62); ++j) {
            levels.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_HIGH), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(AVC_PROFILE_BASELINE), levels));
    }
}

#ifdef SUPPORT_CODEC_VC1
void GetVc1CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.alignment.width = VC1_ALIGNMENT_SIZE;
        capsData.alignment.height = VC1_ALIGNMENT_SIZE;
        capsData.width.maxVal = VC1_MAX_WIDTH_SIZE;
        capsData.height.maxVal = VC1_MAX_HEIGHT_SIZE;
        capsData.bitrate.maxVal = VC1_BITRATE_MAX_SIZE;
        capsData.blockPerFrame.maxVal = VC1_BLOCKPERFRAME_SIZE;
        capsData.blockPerSecond.maxVal = VC1_BLOCKPERSEC_SIZE;
        capsData.pixFormat = {
            static_cast<int32_t>(VideoPixelFormat::YUVI420), static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21)};
        capsData.profiles = {static_cast<int32_t>(VC1_PROFILE_SIMPLE), static_cast<int32_t>(VC1_PROFILE_MAIN),
                             static_cast<int32_t>(VC1_PROFILE_ADVANCED)};
        std::vector<int32_t> advlevels;
        for (int32_t advcount = static_cast<int32_t>(VC1Level::VC1_LEVEL_L0);
            advcount <= static_cast<int32_t>(VC1Level::VC1_LEVEL_L4); ++advcount) {
            advlevels.emplace_back(advcount);
        }
        std::vector<int32_t> levels;
        for (int32_t levelcount = static_cast<int32_t>(VC1_LEVEL_LOW);
            levelcount <= static_cast<int32_t>(VC1Level::VC1_LEVEL_HIGH); ++levelcount) {
            levels.emplace_back(levelcount);
        }
        std::vector<int32_t> simplelevels;
        for (int32_t simplecount = static_cast<int32_t>(VC1_LEVEL_LOW);
            simplecount <= static_cast<int32_t>(VC1Level::VC1_LEVEL_MEDIUM); ++simplecount) {
            simplelevels.emplace_back(simplecount);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VC1_PROFILE_SIMPLE), simplelevels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VC1_PROFILE_MAIN), levels));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(VC1_PROFILE_ADVANCED), advlevels));
    }
}
#endif

void GetMsVideo1CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.width.minVal = MSVIDEO1_MIN_WIDTH_SIZE;
        capsData.height.minVal = MSVIDEO1_MIN_HEIGHT_SIZE;
        capsData.blockPerSecond.maxVal = MSVIDEO1_BLOCKPERSEC_SIZE;
        capsData.pixFormat = {
            static_cast<int32_t>(VideoPixelFormat::RGBA), static_cast<int32_t>(VideoPixelFormat::NV12),
            static_cast<int32_t>(VideoPixelFormat::NV21)};
    }
}

void GetWmv3CapProf(std::vector<CapabilityData> &capaArray)
{
    if (!capaArray.empty()) {
        CapabilityData& capsData = capaArray.back();
        capsData.alignment.width = WMV3_ALIGNMENT_SIZE;
        capsData.alignment.height = WMV3_ALIGNMENT_SIZE;
        capsData.width.minVal = WMV3_MIN_WIDTH_SIZE;
        capsData.height.minVal = WMV3_MIN_HEIGHT_SIZE;
        capsData.width.maxVal = WMV3_MAX_WIDTH_SIZE;
        capsData.height.maxVal = WMV3_MAX_HEIGHT_SIZE;
        capsData.bitrate.maxVal = WMV3_BITRATE_MAX_SIZE;
        capsData.blockPerFrame.maxVal = WMV3_MAX_BLOCKPERFRAME_SIZE;
        capsData.blockPerSecond.maxVal = WMV3_MAX_BLOCKPERSEC_SIZE;
        capsData.frameRate.maxVal = WMV3_MAX_FRAMERATE_SIZE;
        capsData.supportSwapWidthHeight = true;
        capsData.profiles = {static_cast<int32_t>(WMV3_PROFILE_SIMPLE), static_cast<int32_t>(WMV3_PROFILE_MAIN)};
        std::vector<int32_t> levels_s;
        std::vector<int32_t> levels_m;
        for (int32_t j = 0; j <= static_cast<int32_t>(WMV3Level::WMV3_LEVEL_MEDIUM); ++j) {
            levels_s.emplace_back(j);
        }
        for (int32_t j = 0; j <= static_cast<int32_t>(WMV3Level::WMV3_LEVEL_HIGH); ++j) {
            levels_m.emplace_back(j);
        }
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(WMV3_PROFILE_SIMPLE), levels_s));
        capsData.profileLevelsMap.insert(std::make_pair(static_cast<int32_t>(WMV3_PROFILE_MAIN), levels_m));
    }
}

void GetCapabilityData(CapabilityData &capsData, uint32_t index)
{
    capsData.codecName = static_cast<std::string>(SUPPORT_VCODEC[index].codecName);
    capsData.mimeType = static_cast<std::string>(SUPPORT_VCODEC[index].mimeType);
    capsData.codecType = AVCODEC_TYPE_VIDEO_DECODER;
    capsData.isVendor = false;
    capsData.maxInstance = VIDEO_INSTANCE_SIZE;

    capsData.alignment.width = VIDEO_ALIGNMENT_SIZE;
    capsData.alignment.height = VIDEO_ALIGNMENT_SIZE;
    capsData.width.minVal = VIDEO_MIN_SIZE;
    capsData.height.minVal = VIDEO_MIN_SIZE;
    capsData.width.maxVal = VIDEO_MAX_WIDTH_SIZE;
    capsData.height.maxVal = VIDEO_MAX_HEIGHT_SIZE;
    capsData.frameRate.minVal = 0;
    capsData.frameRate.maxVal = VIDEO_FRAMERATE_DEFAULT_SIZE;
    capsData.bitrate.minVal = 1;
    capsData.bitrate.maxVal = VIDEO_BITRATE_MAX_SIZE;
    capsData.blockPerFrame.minVal = 1;
    capsData.blockPerFrame.maxVal = VIDEO_BLOCKPERFRAME_SIZE;
    capsData.blockPerSecond.minVal = 1;
    capsData.blockPerSecond.maxVal = VIDEO_BLOCKPERSEC_SIZE;
    capsData.blockSize.width = VIDEO_ALIGN_SIZE;
    capsData.blockSize.height = VIDEO_ALIGN_SIZE;

    capsData.pixFormat = {
        static_cast<int32_t>(VideoPixelFormat::YUVI420), static_cast<int32_t>(VideoPixelFormat::NV12),
        static_cast<int32_t>(VideoPixelFormat::NV21), static_cast<int32_t>(VideoPixelFormat::RGBA)};
    capsData.graphicPixFormat = {
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P),
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP),
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP),
        static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888)};
}

int32_t FCodec::GetCodecCapability(std::vector<CapabilityData> &capaArray)
{
    for (uint32_t i = 0; i < SUPPORT_VCODEC_NUM; ++i) {
        CapabilityData capsData;
        GetCapabilityData(capsData, i);
        if (capsData.mimeType == "video/mpeg2") {
            capaArray.emplace_back(capsData);
            GetMpeg2CapProf(capaArray);
        } else if (capsData.mimeType == "video/mp4v-es") {
            capaArray.emplace_back(capsData);
            GetMpeg4esCapProf(capaArray);
        } else if (capsData.mimeType == "video/h263") {
            capaArray.emplace_back(capsData);
            GetH263CapProf(capaArray);
        } else if (capsData.mimeType == "video/mjpeg") {
            capaArray.emplace_back(capsData);
#ifdef SUPPORT_CODEC_VC1
        } else if (capsData.mimeType == "video/vc1") {
            capaArray.emplace_back(capsData);
            GetVc1CapProf(capaArray);
#endif
        } else if (capsData.mimeType == "video/msvideo1") {
            capaArray.emplace_back(capsData);
            GetMsVideo1CapProf(capaArray);
        } else if (capsData.mimeType == "video/wmv3") {
            capaArray.emplace_back(capsData);
            GetWmv3CapProf(capaArray);
        } else {
            capsData.frameRate.maxVal = VIDEO_FRAMERATE_MAX_SIZE;
            capaArray.emplace_back(capsData);
            GetAvcCapProf(capaArray);
        }
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS