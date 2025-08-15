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
#include <algorithm>
#include "syspara/parameters.h"
#include "hdf_base.h"
#include "iservmgr_hdi.h"
#include "hcodec_log.h"
#include "type_converter.h"
#include "avcodec_info.h"
#include "meta/meta.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace CodecHDI;
using namespace OHOS::HDI::ServiceManager::V1_0;

static mutex g_mtx;
static sptr<ICodecComponentManager> g_compMgrIpc;
static sptr<ICodecComponentManager> g_compMgrPassthru;

class Listener : public ServStatListenerStub {
public:
    void OnReceive(const ServiceStatus &status) override
    {
        if (status.serviceName == "codec_component_manager_service" && status.status == SERVIE_STATUS_STOP) {
            LOGW("codec_component_manager_service died");
            lock_guard<mutex> lk(g_mtx);
            g_compMgrIpc = nullptr;
        }
    }
};

static bool DecideMode(bool supportPassthrough, bool isSecure)
{
#ifdef BUILD_ENG_VERSION
    string mode = OHOS::system::GetParameter("hcodec.usePassthrough", "");
    if (mode == "1") {
        LOGI("force passthrough");
        return true;
    } else if (mode == "0") {
        LOGI("force ipc");
        return false;
    } else if (mode == "-1") {
        if (isSecure) {
            LOGI("secure, supportPassthrough = %d", supportPassthrough);
            return supportPassthrough;
        } else {
            static int g_cnt = 0;
            bool passthrough = (g_cnt++ % 3 == 0);
            LOGI("g_cnt=%d, %s mode", g_cnt, passthrough ? "passthrough" : "ipc");
            return passthrough;
        }
    }
#endif
    LOGD("supportPassthrough = %d", supportPassthrough);
    return supportPassthrough;
}

sptr<ICodecComponentManager> GetManager(bool getCap, bool supportPassthrough, bool isSecure)
{
    lock_guard<mutex> lk(g_mtx);
    bool isPassthrough = getCap ? true : DecideMode(supportPassthrough, isSecure);
    sptr<ICodecComponentManager>& mng = (isPassthrough ? g_compMgrPassthru : g_compMgrIpc);
    if (mng) {
        return mng;
    }
    LOGI("need to get ICodecComponentManager");
    if (!isPassthrough) {
        sptr<IServiceManager> serviceMng = IServiceManager::Get();
        if (serviceMng) {
            serviceMng->RegisterServiceStatusListener(new Listener(), DEVICE_CLASS_DEFAULT);
        }
    }
    mng = ICodecComponentManager::Get(isPassthrough);
    return mng;
}

vector<CodecCompCapability> GetCapList()
{
    sptr<ICodecComponentManager> mnger = GetManager(true);
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
        LOGD("GetComponentCapabilityList return %zu components", capList.size());
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

VideoFeature HCodecList::FindFeature(const vector<VideoFeature> &features, const enum VideoFeatureKey &key)
{
    auto it = find_if(features.begin(), features.end(),
        [&key](const VideoFeature& feature) { return feature.key == key; });
    if (it == features.end()) {
        return {key, false, {}};
    }
    return *it;
}

bool HCodecList::IsSupportedVideoCodec(const CodecCompCapability &hdiCap)
{
    if (hdiCap.role == MEDIA_ROLETYPE_VIDEO_AVC || hdiCap.role == MEDIA_ROLETYPE_VIDEO_HEVC ||
        hdiCap.role == MEDIA_ROLETYPE_VIDEO_VVC) {
        return true;
    }
    return false;
}

CapabilityData HCodecList::HdiCapToUserCap(const CodecCompCapability &hdiCap)
{
    constexpr int32_t MAX_ENCODE_QUALITY = 100;
    constexpr int32_t MAX_ENCODE_SQRFACTOR = 51;
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
    if (IsSupportSQR(userCap.bitrateMode)) {
        userCap.maxBitrate = {hdiCap.bitRate.min, hdiCap.bitRate.max};
        userCap.sqrFactor =  {0, MAX_ENCODE_SQRFACTOR};
    }
    GetCodecProfileLevels(hdiCap, userCap);
    userCap.measuredFrameRate = GetMeasuredFrameRate(hdiVideoCap);
    userCap.supportSwapWidthHeight = hdiCap.canSwapWidthHeight;
    userCap.encodeQuality = {0, MAX_ENCODE_QUALITY};
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
    GetSupportedFeatureParam(hdiVideoCap, userCap);
    GetSupportedLtrFeatureParam(hdiVideoCap, userCap);
    GetSupportedBFrameFeatureParam(hdiVideoCap, userCap);
    LOGI("isSupportPassthrough: %d", FindFeature(hdiVideoCap.features, VIDEO_FEATURE_PASS_THROUGH).support);
    return userCap;
}

vector<int32_t> HCodecList::GetSupportedBitrateMode(const CodecVideoPortCap& hdiVideoCap)
{
    vector<int32_t> vec;
    for (BitRateMode mode : hdiVideoCap.bitRatemode) {
        OMX_VIDEO_CONTROLRATETYPE omxMode = static_cast<OMX_VIDEO_CONTROLRATETYPE>(mode);
        optional<VideoEncodeBitrateMode> innerMode = TypeConverter::OmxBitrateModeToInnerMode(omxMode);
        if (innerMode.has_value()) {
            vec.push_back(innerMode.value());
            LOGI("support (inner) bitRateMode %d", innerMode.value());
        }
    }
    return vec;
}

bool HCodecList::IsSupportSQR(const vector<int32_t>& supportBitrateMode)
{
    VideoEncodeBitrateMode innerMode = SQR;
    auto it = std::find(supportBitrateMode.begin(), supportBitrateMode.end(),  static_cast<int32_t>(innerMode));
    if (it != supportBitrateMode.end()) {
        LOGI("support SQR bitRateMode!");
        return true;
    }
    return false;
}

vector<int32_t> HCodecList::GetSupportedFormat(const CodecVideoPortCap& hdiVideoCap)
{
    vector<int32_t> vec;
    for (int32_t fmt : hdiVideoCap.supportPixFmts) {
        optional<VideoPixelFormat> innerFmt =
            TypeConverter::DisplayFmtToInnerFmt(static_cast<GraphicPixelFormat>(fmt));
        if (innerFmt.has_value() &&
            find(vec.begin(), vec.end(), static_cast<int32_t>(innerFmt.value())) == vec.end()) {
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

        if (hdiCap.role != MEDIA_ROLETYPE_VIDEO_VVC) {
            continue;
        }
        optional<int32_t> innerProfileVvc;
        optional<int32_t> innerLevelVvc;
        innerProfileVvc = TypeConverter::OmxVvcProfileToInnerProfile(static_cast<CodecVvcProfile>(profile));
        innerLevelVvc = TypeConverter::OmxVvcLevelToInnerLevel(static_cast<CodecVvcLevel>(maxLevel));
        if (innerProfileVvc.has_value() && innerLevelVvc.has_value() && innerLevelVvc.value() >= 0) {
            userCap.profiles.emplace_back(innerProfileVvc.value());
            optional<vector<int32_t>> allLevel =
                TypeConverter::InnerVvcMaxLevelToAllLevels(static_cast<VVCLevel>(innerLevelVvc.value()));
            if (allLevel.has_value()) {
                userCap.profileLevelsMap[innerProfileVvc.value()] = allLevel.value();
            }
            LOGI("role %d support (inner) profile %d and level up to %d",
                hdiCap.role, innerProfileVvc.value(), innerLevelVvc.value());
        }
    }
}

void HCodecList::GetSupportedFeatureParam(const CodecVideoPortCap& hdiVideoCap,
                                          CapabilityData& userCap)
{
    // convert CodecHDI to AVCodec Feature
    map<CodecHDI::VideoFeatureKey, pair<AVCapabilityFeature, string>> featureConvertMap = {
        {   CodecHDI::VIDEO_FEATURE_LOW_LATENCY,
            {AVCapabilityFeature::VIDEO_LOW_LATENCY, "isSupportLowLatency"}
        },
        {   CodecHDI::VIDEO_FEATURE_TSVC,
            {AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, "isSupportTSVC"}
        },
        {   CodecHDI::VIDEO_FEATURE_WATERMARK,
            {AVCapabilityFeature::VIDEO_WATERMARK, "isSupportWaterMark"}
        },
        {   CodecHDI::VIDEO_FEATURE_SEEK_WITHOUT_FLUSH,
            {AVCapabilityFeature::VIDEO_DECODER_SEEK_WITHOUT_FLUSH, "isSupportSeekWithoutFlush"}
        },
        {   CodecHDI::VIDEO_FEATURE_QP_MAP,
            {AVCapabilityFeature::VIDEO_ENCODER_QP_MAP, "isSupportQPMap"}
        },
    };

    for (auto featureInfo: featureConvertMap) {
        VideoFeature feature = FindFeature(hdiVideoCap.features, featureInfo.first);
        if (feature.support) {
            userCap.featuresMap[static_cast<int32_t>(featureInfo.second.first)] = Format();
        }
        LOGI("%s: %d", featureInfo.second.second.c_str(), feature.support);
    }
}

void HCodecList::GetSupportedLtrFeatureParam(const CodecVideoPortCap& hdiVideoCap,
                                             CapabilityData& userCap)
{
    VideoFeature feature = FindFeature(hdiVideoCap.features, VIDEO_FEATURE_LTR);
    if (!feature.support || feature.extendInfo.empty()) {
        LOGI("isSupportLTR: 0");
        return;
    }
    Format format;
    int32_t maxLTRFrameNum = feature.extendInfo[0];
    if (maxLTRFrameNum >= 0) {
        format.PutIntValue(OHOS::Media::Tag::FEATURE_PROPERTY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, maxLTRFrameNum);
        userCap.featuresMap[static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE)] = format;
        LOGI("isSupportLTR: 1, maxLTRFrameNum: %d", maxLTRFrameNum);
        return;
    }
    LOGI("isSupportLTR: 0");
}

void HCodecList::GetSupportedBFrameFeatureParam(const CodecVideoPortCap& hdiVideoCap,
                                                CapabilityData& userCap)
{
    VideoFeature feature = FindFeature(hdiVideoCap.features, VIDEO_FEATURE_ENCODE_B_FRAME);
    if (!feature.support || feature.extendInfo.empty()) {
        LOGI("isSupportBFrame: 0");
        return;
    }
    Format format;
    int32_t maxBFrameCount = feature.extendInfo[0];
    if (maxBFrameCount > 0) {
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_MAX_B_FRAME, maxBFrameCount);
        userCap.featuresMap[static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_B_FRAME)] = format;
        LOGI("isSupportBFrame: 1, maxSupportBFrameCnt: %u", maxBFrameCount);
        return;
    }
    LOGI("isSupportBFrame: 0");
    return;
}
} // namespace OHOS::MediaAVCodec