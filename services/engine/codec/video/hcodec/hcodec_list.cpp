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

#include "hcodec_list.h"
#include <map>
#include "syspara/parameters.h"
#include "utils/hdf_base.h"
#include "hcodec_log.h"
#include "type_converter.h"
#include "avcodec_info.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V1_0;

sptr<ICodecComponentManager> GetManager()
{
    bool usePassthrough = OHOS::system::GetBoolParameter("hcodec.usePassthrough", false);
    LOGI("%{public}s mode", usePassthrough ? "passthrough" : "ipc");
    static sptr<ICodecComponentManager> compMgr = ICodecComponentManager::Get(usePassthrough);
    return compMgr;
}

vector<CodecCompCapability> GetCapList()
{
    sptr<ICodecComponentManager> mnger = GetManager();
    if (mnger == nullptr) {
        LOGE("failed to create codec component manager");
        return {};
    }
    int32_t compCnt = 0;
    int32_t ret = mnger->GetComponentNum(compCnt);
    if (ret != HDF_SUCCESS || compCnt <= 0) {
        LOGE("failed to query component number, ret=%{public}d", ret);
        return {};
    }
    std::vector<CodecCompCapability> capList(compCnt);
    ret = mnger->GetComponentCapabilityList(capList, compCnt);
    if (ret != HDF_SUCCESS) {
        LOGE("failed to query component capability list, ret=%{public}d", ret);
        return {};
    }
    return capList;
}

int32_t HCodecList::GetCapabilityList(std::vector<CapabilityData>& caps)
{
    caps.clear();
    vector<CodecCompCapability> capList = GetCapList();
    for (const CodecCompCapability& one : capList) {
        if (IsSupportedVideoCodec(one)) {
            caps.emplace_back(HdiCapToUserCap(one));
        }
    }
    return AVCS_ERR_OK;
}

bool HCodecList::IsSupportedVideoCodec(const CodecCompCapability &hdiCap)
{
    if (hdiCap.role == MEDIA_ROLETYPE_VIDEO_AVC || hdiCap.role == MEDIA_ROLETYPE_VIDEO_HEVC) {
        return true;
    }
    return false;
}

CapabilityData HCodecList::HdiCapToUserCap(const CodecCompCapability &hdiCap)
{
    const CodecVideoPortCap& hdiVideoCap = hdiCap.port.video;
    CapabilityData userCap;
    userCap.codecName = hdiCap.compName;
    userCap.codecType = TypeConverter::HdiCodecTypeToInnerCodecType(hdiCap.type).value_or(AVCODEC_TYPE_NONE);
    userCap.mimeType = TypeConverter::HdiRoleToMime(hdiCap.role);
    userCap.isVendor = true;
    userCap.maxInstance = hdiCap.maxInst;
    userCap.bitrate = {hdiCap.bitRate.min, hdiCap.bitRate.max};
    userCap.alignment = {hdiVideoCap.whAlignment.widthAlignment, hdiVideoCap.whAlignment.heightAlignment};
    userCap.width = {hdiVideoCap.minSize.width, hdiVideoCap.maxSize.width};
    userCap.height = {hdiVideoCap.minSize.height, hdiVideoCap.maxSize.height};
    userCap.frameRate = {hdiVideoCap.frameRate.min, hdiVideoCap.frameRate.max};
    userCap.blockPerFrame = {hdiVideoCap.blockCount.min, hdiVideoCap.blockCount.max};
    userCap.blockPerSecond = {hdiVideoCap.blocksPerSecond.min, hdiVideoCap.blocksPerSecond.max};
    userCap.blockSize = {hdiVideoCap.blockSize.width, hdiVideoCap.blockSize.height};
    userCap.pixFormat = GetSupportedFormat(hdiVideoCap);
    userCap.bitrateMode = GetSupportedBitrateMode(hdiVideoCap);
    userCap.profileLevelsMap = GetCodecProfileLevels(hdiCap);
    userCap.measuredFrameRate = GetMeasuredFrameRate(hdiVideoCap);
    userCap.supportSwapWidthHeight = hdiCap.canSwapWidthHeight;
    return userCap;
}

vector<int32_t> HCodecList::GetSupportedBitrateMode(const CodecVideoPortCap& hdiVideoCap)
{
    vector<int32_t> vec;
    for (BitRateMode mode : hdiVideoCap.bitRatemode) {
        optional<VideoEncodeBitrateMode> innerMode = TypeConverter::HdiBitrateModeToInnerMode(mode);
        if (innerMode.has_value()) {
            vec.push_back(innerMode.value());
        }
    }
    return vec;
}

vector<int32_t> HCodecList::GetSupportedFormat(const CodecVideoPortCap& hdiVideoCap)
{
    vector<int32_t> vec;
    for (int32_t fmt : hdiVideoCap.supportPixFmts) {
        optional<VideoPixelFormat> innerFmt =
            TypeConverter::DisplayFmtToInnerFmt(static_cast<GraphicPixelFormat>(fmt));
        if (innerFmt.has_value()) {
            vec.push_back(innerFmt.value());
        }
    }
    return vec;
}

map<ImgSize, Range> HCodecList::GetMeasuredFrameRate(const CodecVideoPortCap& hdiVideoCap)
{
    enum MeasureStep {
        WIDTH = 0,
        HEIGHT = 1,
        MIN_RATE = 2,
        MAX_RATE = 3,
        STEP_BUTT = 4
    };
    map<ImgSize, Range> userRateMap;
    for (size_t index = 0; index < hdiVideoCap.measuredFrameRate.size(); index += STEP_BUTT) {
        if (hdiVideoCap.measuredFrameRate[index] <= 0) {
            continue;
        }
        ImgSize imageSize(hdiVideoCap.measuredFrameRate[index + WIDTH], hdiVideoCap.measuredFrameRate[index + HEIGHT]);
        Range range(hdiVideoCap.measuredFrameRate[index + MIN_RATE], hdiVideoCap.measuredFrameRate[index + MAX_RATE]);
        userRateMap[imageSize] = range;
    }
    return userRateMap;
}

map<int32_t, vector<int32_t>> HCodecList::GetCodecProfileLevels(const CodecCompCapability& hdiCap)
{
    map<int32_t, vector<int32_t>> userProfileLevelMap;
    for (const int32_t& hdiProfile : hdiCap.supportProfiles) {
        if (hdiProfile <= 0) {
            continue;
        }
        optional<int32_t> userProfile = std::nullopt;
        vector<int32_t> userLevel;
        if (hdiCap.role == MEDIA_ROLETYPE_VIDEO_AVC) {
            userProfile = TypeConverter::HdiAvcProfileToAvcProfile(static_cast<Profile>(hdiProfile));
            userLevel = {AVC_LEVEL_1,
                         AVC_LEVEL_1b,
                         AVC_LEVEL_11,
                         AVC_LEVEL_12,
                         AVC_LEVEL_13,
                         AVC_LEVEL_2,
                         AVC_LEVEL_21,
                         AVC_LEVEL_22,
                         AVC_LEVEL_3,
                         AVC_LEVEL_31,
                         AVC_LEVEL_32,
                         AVC_LEVEL_4,
                         AVC_LEVEL_41,
                         AVC_LEVEL_42,
                         AVC_LEVEL_5,
                         AVC_LEVEL_51};
        } else if (hdiCap.role == MEDIA_ROLETYPE_VIDEO_HEVC) {
            userProfile = TypeConverter::HdiHevcProfileToHevcProfile(static_cast<Profile>(hdiProfile));
            userLevel = {HEVC_LEVEL_1,
                         HEVC_LEVEL_2,
                         HEVC_LEVEL_21,
                         HEVC_LEVEL_3,
                         HEVC_LEVEL_31,
                         HEVC_LEVEL_4,
                         HEVC_LEVEL_41,
                         HEVC_LEVEL_5,
                         HEVC_LEVEL_51,
                         HEVC_LEVEL_52,
                         HEVC_LEVEL_6,
                         HEVC_LEVEL_61,
                         HEVC_LEVEL_62};
        }
        if (!userProfile.has_value()) {
            LOGW("unsupported hdi profile(0x%{public}x)", hdiProfile);
            continue;
        }
        if (userProfileLevelMap.find(userProfile.value()) == userProfileLevelMap.end()) {
            userProfileLevelMap[userProfile.value()] = userLevel;
        }
    }
    return userProfileLevelMap;
}
} // namespace OHOS::MediaAVCodec