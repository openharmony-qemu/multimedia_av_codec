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
#include <numeric>
#include "syspara/parameters.h"
#include "utils/hdf_base.h"
#include "hdi/iservmgr_hdi.h"
#include "hcodec_log.h"
#include "type_converter.h"
#include "avcodec_info.h"
#include "meta/meta.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::HDI::Codec::V3_0;
using namespace OHOS::HDI::ServiceManager::V1_0;

static mutex g_mtx;
static sptr<ICodecComponentManager> g_compMgr;

class Listener : public ServStatListenerStub {
public:
    void OnReceive(const ServiceStatus &status) override
    {
        if (status.serviceName == "codec_component_manager_service" && status.status == SERVIE_STATUS_STOP) {
            LOGW("codec_component_manager_service died");
            lock_guard<mutex> lk(g_mtx);
            g_compMgr = nullptr;
        }
    }
};

bool IsPassthrough()
{
    static bool usePassthrough = OHOS::system::GetBoolParameter("hcodec.usePassthrough", false);
    LOGI("%s mode", usePassthrough ? "passthrough" : "ipc");
    return usePassthrough;
}

sptr<ICodecComponentManager> GetManager()
{
    lock_guard<mutex> lk(g_mtx);
    if (g_compMgr) {
        return g_compMgr;
    }
    LOGI("need to get ICodecComponentManager");
    bool isPassthrough = IsPassthrough();
    if (!isPassthrough) {
        sptr<IServiceManager> serviceMng = IServiceManager::Get();
        if (serviceMng) {
            serviceMng->RegisterServiceStatusListener(new Listener(), DEVICE_CLASS_DEFAULT);
        }
    }
    g_compMgr = ICodecComponentManager::Get(isPassthrough);
    return g_compMgr;
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
        LOGE("failed to query component number, ret=%d", ret);
        return {};
    }
    std::vector<CodecCompCapability> capList(compCnt);
    ret = mnger->GetComponentCapabilityList(capList, compCnt);
    if (ret != HDF_SUCCESS) {
        LOGE("failed to query component capability list, ret=%d", ret);
        return {};
    }
    if (capList.empty()) {
        LOGE("GetComponentCapabilityList return empty");
    } else {
        LOGI("GetComponentCapabilityList return %zu components", capList.size());
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
    constexpr int32_t MAX_ENCODE_QUALITY = 100;
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
    GetCodecProfileLevels(hdiCap, userCap);
    userCap.measuredFrameRate = GetMeasuredFrameRate(hdiVideoCap);
    userCap.supportSwapWidthHeight = hdiCap.canSwapWidthHeight;
    userCap.encodeQuality = {0, MAX_ENCODE_QUALITY};
    GetSupportedFeatureParam(hdiVideoCap, userCap);
    LOGI("----- codecName: %s -----", userCap.codecName.c_str());
    LOGI("codecType: %d, mimeType: %s, maxInstance %d",
        userCap.codecType, userCap.mimeType.c_str(), userCap.maxInstance);
    LOGI("bitrate: [%d, %d], alignment: [%d x %d]",
        userCap.bitrate.minVal, userCap.bitrate.maxVal, userCap.alignment.width, userCap.alignment.height);
    LOGI("width: [%d, %d], height: [%d, %d]",
        userCap.width.minVal, userCap.width.maxVal, userCap.height.minVal, userCap.height.maxVal);
    LOGI("frameRate: [%d, %d], blockSize: [%d x %d]",
        userCap.frameRate.minVal, userCap.frameRate.maxVal, userCap.blockSize.width, userCap.blockSize.height);
    LOGI("blockPerFrame: [%d, %d], blockPerSecond: [%d, %d]",
        userCap.blockPerFrame.minVal, userCap.blockPerFrame.maxVal,
        userCap.blockPerSecond.minVal, userCap.blockPerSecond.maxVal);
    LOGI("isSupportLowLatency: %d, isSupportTSVC: %d, isSupportLTR %d and maxLTRFrameNum %d",
        hdiVideoCap.isSupportLowLatency, hdiVideoCap.isSupportTSVC,
        hdiVideoCap.isSupportLTR, hdiVideoCap.maxLTRFrameNum);
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
            vec.push_back(static_cast<int32_t>(innerFmt.value()));
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

void HCodecList::GetCodecProfileLevels(const CodecCompCapability& hdiCap, CapabilityData& userCap)
{
    for (size_t i = 0; i + 1 < hdiCap.supportProfiles.size(); i += 2) { // 2 means profile & level pair
        int32_t profile = hdiCap.supportProfiles[i];
        int32_t maxLevel = hdiCap.supportProfiles[i + 1];
        optional<int32_t> innerProfile;
        optional<int32_t> innerLevel;
        if (hdiCap.role == MEDIA_ROLETYPE_VIDEO_AVC) {
            innerProfile = TypeConverter::OmxAvcProfileToInnerProfile(static_cast<OMX_VIDEO_AVCPROFILETYPE>(profile));
            innerLevel = TypeConverter::OmxAvcLevelToInnerLevel(static_cast<OMX_VIDEO_AVCLEVELTYPE>(maxLevel));
        } else if (hdiCap.role == MEDIA_ROLETYPE_VIDEO_HEVC) {
            innerProfile = TypeConverter::OmxHevcProfileToInnerProfile(static_cast<CodecHevcProfile>(profile));
            innerLevel = TypeConverter::OmxHevcLevelToInnerLevel(static_cast<CodecHevcLevel>(maxLevel));
        }
        if (innerProfile.has_value() && innerLevel.has_value() && innerLevel.value() >= 0) {
            userCap.profiles.emplace_back(innerProfile.value());
            vector<int32_t> allLevel(innerLevel.value() + 1);
            std::iota(allLevel.begin(), allLevel.end(), 0);
            userCap.profileLevelsMap[innerProfile.value()] = allLevel;
            LOGI("role %d support (inner) profile %d and level up to %d",
                hdiCap.role, innerProfile.value(), innerLevel.value());
        }
    }
}

void HCodecList::GetSupportedFeatureParam(const OHOS::HDI::Codec::V3_0::CodecVideoPortCap& hdiVideoCap,
                                          CapabilityData& userCap)
{
    if (hdiVideoCap.isSupportLowLatency) {
        userCap.featuresMap[static_cast<int32_t>(AVCapabilityFeature::VIDEO_LOW_LATENCY)] = Format();
    }
    if (hdiVideoCap.isSupportTSVC) {
        userCap.featuresMap[static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY)] = Format();
    }
    if (hdiVideoCap.isSupportLTR) {
        Format format;
        int32_t maxLTRFrameNum = hdiVideoCap.maxLTRFrameNum;
        if (maxLTRFrameNum >= 0) {
            format.PutIntValue(OHOS::Media::Tag::FEATURE_PROPERTY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, maxLTRFrameNum);
            userCap.featuresMap[static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE)] = format;
        }
    }
}
} // namespace OHOS::MediaAVCodec