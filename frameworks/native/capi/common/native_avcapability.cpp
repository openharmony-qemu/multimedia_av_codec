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

#include <algorithm>
#include "native_avmagic.h"
#include "avcodec_list.h"
#include "avcodec_info.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "securec.h"
#include "native_avcapability.h"
#include "common/native_mfmagic.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "NativeAVCapability"};
constexpr uint32_t MAX_LENGTH = 255;
}
using namespace OHOS::MediaAVCodec;

OH_AVCapability::~OH_AVCapability() {}

OH_AVCapability *OH_AVCodec_GetCapability(const char *mime, bool isEncoder)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Get capability failed: mime is nullptr");
    CHECK_AND_RETURN_RET_LOG(strlen(mime) != 0 && strlen(mime) < MAX_LENGTH, nullptr,
        "Get capability failed: invalid mime strlen, %{public}zu", strlen(mime));
    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, nullptr, "Get capability failed: CreateAVCodecList failed");
    uint32_t sizeOfCap = sizeof(OH_AVCapability);
    CapabilityData *capabilityData = codeclist->GetCapability(mime, isEncoder, AVCodecCategory::AVCODEC_NONE);
    CHECK_AND_RETURN_RET_LOG(capabilityData != nullptr, nullptr,
                             "Get capability failed: cannot find matched capability");
    const std::string &name = capabilityData->codecName;
    CHECK_AND_RETURN_RET_LOG(!name.empty(), nullptr, "Get capability failed: cannot find matched capability");
    void *addr = codeclist->GetBuffer(name, sizeOfCap);
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, nullptr, "Get capability failed: malloc capability buffer failed");
    OH_AVCapability *obj = static_cast<OH_AVCapability *>(addr);
    obj->magic_ = AVMagic::AVCODEC_MAGIC_AVCAPABILITY;
    obj->capabilityData_ = capabilityData;
    AVCODEC_LOGD("OH_AVCodec_GetCapability successful");
    return obj;
}

OH_AVCapability *OH_AVCodec_GetCapabilityByCategory(const char *mime, bool isEncoder, OH_AVCodecCategory category)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "Get capabilityByCategory failed: mime is nullptr");
    CHECK_AND_RETURN_RET_LOG(strlen(mime) != 0 && strlen(mime) < MAX_LENGTH, nullptr,
        "Get capabilityByCategory failed: invalid mime strlen, %{public}zu", strlen(mime));
    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, nullptr,
        "Get capabilityByCategory failed: CreateAVCodecList failed");
    AVCodecCategory innerCategory;
    if (category == HARDWARE) {
        innerCategory = AVCodecCategory::AVCODEC_HARDWARE;
    } else if (category == SOFTWARE) {
        innerCategory = AVCodecCategory::AVCODEC_SOFTWARE;
    } else {
        AVCODEC_LOGE("Unsupported category {public}%d", static_cast<int32_t>(category));
        return nullptr;
    }
    uint32_t sizeOfCap = sizeof(OH_AVCapability);
    CapabilityData *capabilityData = codeclist->GetCapability(mime, isEncoder, innerCategory);
    CHECK_AND_RETURN_RET_LOG(capabilityData != nullptr, nullptr,
                             "Get capabilityByCategory failed: cannot find matched capability");
    const std::string &name = capabilityData->codecName;
    CHECK_AND_RETURN_RET_LOG(!name.empty(), nullptr, "Get capabilityByCategory failed: cannot find matched capability");
    void *addr = codeclist->GetBuffer(name, sizeOfCap);
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, nullptr,
                             "Get capabilityByCategory failed: malloc capability buffer failed");
    OH_AVCapability *obj = static_cast<OH_AVCapability *>(addr);
    obj->magic_ = AVMagic::AVCODEC_MAGIC_AVCAPABILITY;
    obj->capabilityData_ = capabilityData;
    AVCODEC_LOGD("OH_AVCodec_GetCapabilityByCategory successful");
    return obj;
}

const char *OH_AVCapability_GetName(OH_AVCapability *capability)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        "", "Invalid parameter");
    const auto &name = capability->capabilityData_->codecName;
    return name.data();
}

bool OH_AVCapability_IsHardware(OH_AVCapability *capability)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        false, "Invalid parameter");
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capability->capabilityData_);
    return codecInfo->IsHardwareAccelerated();
}

int32_t OH_AVCapability_GetMaxSupportedInstances(OH_AVCapability *capability)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        0, "Invalid parameter");
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capability->capabilityData_);
    return codecInfo->GetMaxSupportedInstances();
}

OH_AVErrCode OH_AVCapability_GetSupportedProfiles(OH_AVCapability *capability, const int32_t **profiles,
                                                  uint32_t *profileNum)
{
    CHECK_AND_RETURN_RET_LOG(profileNum != nullptr && profiles != nullptr, AV_ERR_INVALID_VAL,
                             "Get supported profiles failed: null input");
    *profiles = nullptr;
    *profileNum = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capability->capabilityData_);
    const auto &vec = codecInfo->GetSupportedProfiles();
    if (vec.size() == 0) {
        return AV_ERR_OK;
    }

    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, AV_ERR_UNKNOWN,
        "Get supported profiles failed: CreateAVCodecList failed");
    size_t vecSize = vec.size() * sizeof(int32_t);
    int32_t *buf = static_cast<int32_t *>(codeclist->NewBuffer(vecSize));
    CHECK_AND_RETURN_RET_LOG(buf != nullptr, AV_ERR_NO_MEMORY, "new buffer failed");
    errno_t ret = memcpy_s(buf, vecSize, vec.data(), vecSize);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_UNKNOWN, "memcpy_s failed");

    *profiles = buf;
    *profileNum = vec.size();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetSupportedLevelsForProfile(OH_AVCapability *capability, int32_t profile,
                                                          const int32_t **levels, uint32_t *levelNum)
{
    CHECK_AND_RETURN_RET_LOG(levels != nullptr && levelNum != nullptr, AV_ERR_INVALID_VAL,
                             "Get supported levels for profile failed: null input");
    *levels = nullptr;
    *levelNum = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capability->capabilityData_);
    const auto &profileLevelsMap = codecInfo->GetSupportedLevelsForProfile();
    const auto &levelsmatch = profileLevelsMap.find(profile);
    if (levelsmatch == profileLevelsMap.end()) {
        return AV_ERR_INVALID_VAL;
    }
    const auto &vec = levelsmatch->second;
    if (vec.size() == 0) {
        return AV_ERR_OK;
    }

    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, AV_ERR_UNKNOWN,
        "Get supported levels for profile failed: CreateAVCodecList failed");
    size_t vecSize = vec.size() * sizeof(int32_t);
    int32_t *buf = static_cast<int32_t *>(codeclist->NewBuffer(vecSize));
    CHECK_AND_RETURN_RET_LOG(buf != nullptr, AV_ERR_NO_MEMORY, "new buffer failed");
    errno_t ret = memcpy_s(buf, vecSize, vec.data(), vecSize);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_UNKNOWN, "memcpy_s failed");

    *levels = buf;
    *levelNum = vec.size();
    return AV_ERR_OK;
}

bool OH_AVCapability_AreProfileAndLevelSupported(OH_AVCapability *capability, int32_t profile, int32_t level)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        false, "Invalid parameter");
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capability->capabilityData_);
    const auto &profileLevelsMap = codecInfo->GetSupportedLevelsForProfile();
    const auto &levels = profileLevelsMap.find(profile);
    if (levels == profileLevelsMap.end()) {
        return false;
    }
    return find(levels->second.begin(), levels->second.end(), level) != levels->second.end();
}

OH_AVErrCode OH_AVCapability_GetEncoderBitrateRange(OH_AVCapability *capability, OH_AVRange *bitrateRange)
{
    CHECK_AND_RETURN_RET_LOG(bitrateRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get encoder bitrate range failed: null input");
    bitrateRange->minVal = 0;
    bitrateRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isEncoder(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the encoder capability");
    }
    std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capData);
    const auto &bitrate = codecInfo->GetSupportedBitrate();
    bitrateRange->minVal = bitrate.minVal;
    bitrateRange->maxVal = bitrate.maxVal;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetEncoderQualityRange(OH_AVCapability *capability, OH_AVRange *qualityRange)
{
    CHECK_AND_RETURN_RET_LOG(qualityRange != nullptr, AV_ERR_INVALID_VAL, "Get encoder quality failed: null input");
    qualityRange->minVal = 0;
    qualityRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isEncoder(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the encoder capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &quality = codecInfo->GetSupportedEncodeQuality();
    qualityRange->minVal = quality.minVal;
    qualityRange->maxVal = quality.maxVal;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetEncoderComplexityRange(OH_AVCapability *capability, OH_AVRange *complexityRange)
{
    CHECK_AND_RETURN_RET_LOG(complexityRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get encoder complexity range failed: null input");
    complexityRange->minVal = 0;
    complexityRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isEncoder(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the encoder capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &complexity = codecInfo->GetSupportedComplexity();
    complexityRange->minVal = complexity.minVal;
    complexityRange->maxVal = complexity.maxVal;
    return AV_ERR_OK;
}

bool OH_AVCapability_IsEncoderBitrateModeSupported(OH_AVCapability *capability, OH_BitrateMode bitrateMode)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        false, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isEncoder(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the encoder capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &bitrateModeVec = codecInfo->GetSupportedBitrateMode();
    return find(bitrateModeVec.begin(), bitrateModeVec.end(), bitrateMode) != bitrateModeVec.end();
}

OH_AVErrCode OH_AVCapability_GetAudioSupportedSampleRates(OH_AVCapability *capability, const int32_t **sampleRates,
                                                          uint32_t *sampleRateNum)
{
    CHECK_AND_RETURN_RET_LOG(sampleRates != nullptr && sampleRateNum != nullptr, AV_ERR_INVALID_VAL,
                             "Get audio supported samplerates failed: null input");
    *sampleRates = nullptr;
    *sampleRateNum = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isAudio(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the audio capability");
    }
    std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capData);
    const auto &vec = codecInfo->GetSupportedSampleRates();
    if (vec.size() == 0) {
        return AV_ERR_OK;
    }

    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, AV_ERR_UNKNOWN,
        "Get audio supported samplerates failed: CreateAVCodecList failed");
    size_t vecSize = vec.size() * sizeof(int32_t);
    int32_t *buf = static_cast<int32_t *>(codeclist->NewBuffer(vecSize));
    CHECK_AND_RETURN_RET_LOG(buf != nullptr, AV_ERR_NO_MEMORY, "new buffer failed");
    errno_t ret = memcpy_s(buf, vecSize, vec.data(), vecSize);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_UNKNOWN, "memcpy_s failed");

    *sampleRates = buf;
    *sampleRateNum = vec.size();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetAudioSupportedSampleRateRanges(OH_AVCapability *capability,
                                                               OH_AVRange **sampleRateRanges, uint32_t *rangesNum)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CHECK_AND_RETURN_RET_LOG(sampleRateRanges != nullptr && rangesNum != nullptr, AV_ERR_INVALID_VAL,
                             "Get audio supported samplerate ranges failed: null input");
    *sampleRateRanges = nullptr;
    *rangesNum = 0;
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isAudio(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the audio capability");
    }
    std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capData);
    const std::vector<Range> &vec = codecInfo->GetSupportedSampleRateRanges();

    if (vec.size() == 0) {
        return AV_ERR_OK;
    }
    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, AV_ERR_UNKNOWN,
        "Get audio supported samplerates failed: CreateAVCodecList failed");
    if (capability->sampleRateRanges_ != nullptr) {
        capability->sampleRateRanges_ = nullptr;
    }

    size_t vecSize = vec.size() * sizeof(OH_AVRange);
    capability->sampleRateRanges_ = static_cast<OH_AVRange *>(codeclist->NewBuffer(vecSize));
    CHECK_AND_RETURN_RET_LOG(capability->sampleRateRanges_ != nullptr, AV_ERR_NO_MEMORY, "new buffer failed");
    for (size_t i = 0; i < vec.size(); i++) {
        capability->sampleRateRanges_[i].minVal = vec[i].minVal;
        capability->sampleRateRanges_[i].maxVal = vec[i].maxVal;
    }
    *sampleRateRanges = capability->sampleRateRanges_;
    *rangesNum = vec.size();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetAudioChannelCountRange(OH_AVCapability *capability, OH_AVRange *channelCountRange)
{
    CHECK_AND_RETURN_RET_LOG(channelCountRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get audio channel count range failed: null input");
    channelCountRange->minVal = 0;
    channelCountRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isAudio(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the audio capability");
    }
    std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capData);
    const auto &channels = codecInfo->GetSupportedChannel();
    channelCountRange->minVal = channels.minVal;
    channelCountRange->maxVal = channels.maxVal;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoSupportedPixelFormats(OH_AVCapability *capability, const int32_t **pixFormats,
                                                           uint32_t *pixFormatNum)
{
    CHECK_AND_RETURN_RET_LOG(pixFormats != nullptr && pixFormatNum != nullptr, AV_ERR_INVALID_VAL,
                             "Get video supported pixel formats failed: null input");
    *pixFormats = nullptr;
    *pixFormatNum = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &vec = codecInfo->GetSupportedFormats();
    if (vec.size() == 0) {
        return AV_ERR_OK;
    }
    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET_LOG(codeclist != nullptr, AV_ERR_UNKNOWN,
        "Get video supported pixel formats failed: CreateAVCodecList failed");
    size_t vecSize = vec.size() * sizeof(int32_t);
    int32_t *buf = static_cast<int32_t *>(codeclist->NewBuffer(vecSize));
    CHECK_AND_RETURN_RET_LOG(buf != nullptr, AV_ERR_NO_MEMORY, "new buffer failed");
    errno_t ret = memcpy_s(buf, vecSize, vec.data(), vecSize);
    CHECK_AND_RETURN_RET_LOG(ret == EOK, AV_ERR_UNKNOWN, "memcpy_s failed");
    *pixFormats = buf;
    *pixFormatNum = vec.size();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoWidthAlignment(OH_AVCapability *capability, int32_t *widthAlignment)
{
    CHECK_AND_RETURN_RET_LOG(widthAlignment != nullptr, AV_ERR_INVALID_VAL,
                             "Get video width alignment failed: null input");
    *widthAlignment = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    *widthAlignment = codecInfo->GetSupportedWidthAlignment();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoHeightAlignment(OH_AVCapability *capability, int32_t *heightAlignment)
{
    CHECK_AND_RETURN_RET_LOG(heightAlignment != nullptr, AV_ERR_INVALID_VAL,
                             "Get video height alignment failed: null input");
    *heightAlignment = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    *heightAlignment = codecInfo->GetSupportedHeightAlignment();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoWidthRangeForHeight(OH_AVCapability *capability, int32_t height,
                                                         OH_AVRange *widthRange)
{
    CHECK_AND_RETURN_RET_LOG(widthRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get video width range for height failed: null input");
    widthRange->minVal = 0;
    widthRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &width = codecInfo->GetVideoWidthRangeForHeight(height);
    widthRange->minVal = width.minVal;
    widthRange->maxVal = width.maxVal;
    CHECK_AND_RETURN_RET_LOG(width.minVal != 0 || width.maxVal != 0, AV_ERR_INVALID_VAL, "width range is [0, 0]");
    AVCODEC_LOGD("Success to get width range [%{public}d, %{public}d], by height %{public}d",
        width.minVal, width.maxVal, height);
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoHeightRangeForWidth(OH_AVCapability *capability, int32_t width,
                                                         OH_AVRange *heightRange)
{
    CHECK_AND_RETURN_RET_LOG(heightRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get video height range for width failed: null input");
    heightRange->minVal = 0;
    heightRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &height = codecInfo->GetVideoHeightRangeForWidth(width);
    heightRange->minVal = height.minVal;
    heightRange->maxVal = height.maxVal;
    CHECK_AND_RETURN_RET_LOG(height.minVal != 0 || height.maxVal != 0, AV_ERR_INVALID_VAL, "height range is [0, 0]");
    AVCODEC_LOGD("Success to get height range [%{public}d, %{public}d], by width %{public}d",
        height.minVal, height.maxVal, width);
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoWidthRange(OH_AVCapability *capability, OH_AVRange *widthRange)
{
    CHECK_AND_RETURN_RET_LOG(widthRange != nullptr, AV_ERR_INVALID_VAL, "Get video width range failed: null input");
    widthRange->minVal = 0;
    widthRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &width = codecInfo->GetSupportedWidth();
    widthRange->minVal = width.minVal;
    widthRange->maxVal = width.maxVal;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoHeightRange(OH_AVCapability *capability, OH_AVRange *heightRange)
{
    CHECK_AND_RETURN_RET_LOG(heightRange != nullptr, AV_ERR_INVALID_VAL, "Get video height range failed: null input");
    heightRange->minVal = 0;
    heightRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &height = codecInfo->GetSupportedHeight();
    heightRange->minVal = height.minVal;
    heightRange->maxVal = height.maxVal;
    return AV_ERR_OK;
}

bool OH_AVCapability_IsVideoSizeSupported(OH_AVCapability *capability, int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        false, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capData);
    return videoCap->IsSizeSupported(width, height);
}

OH_AVErrCode OH_AVCapability_GetVideoFrameRateRange(OH_AVCapability *capability, OH_AVRange *frameRateRange)
{
    CHECK_AND_RETURN_RET_LOG(frameRateRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get video framerate range failed: null input");
    frameRateRange->minVal = 0;
    frameRateRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capData);
    const auto &frameRate = videoCap->GetSupportedFrameRate();
    frameRateRange->minVal = frameRate.minVal;
    frameRateRange->maxVal = frameRate.maxVal;
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVCapability_GetVideoFrameRateRangeForSize(OH_AVCapability *capability, int32_t width, int32_t height,
                                                           OH_AVRange *frameRateRange)
{
    CHECK_AND_RETURN_RET_LOG(frameRateRange != nullptr, AV_ERR_INVALID_VAL,
                             "Get video framerate range for size failed: null input");
    frameRateRange->minVal = 0;
    frameRateRange->maxVal = 0;
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        AV_ERR_INVALID_VAL, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capData);
    const auto &frameRate = videoCap->GetSupportedFrameRatesFor(width, height);
    frameRateRange->minVal = frameRate.minVal;
    frameRateRange->maxVal = frameRate.maxVal;
    CHECK_AND_RETURN_RET_LOG(frameRate.minVal != 0 || frameRate.maxVal != 0, AV_ERR_INVALID_VAL,
        "frameRate range is [0, 0]");
    AVCODEC_LOGD("Success to get frameRate range [%{public}d, %{public}d], by width %{public}d and height %{public}d",
        frameRate.minVal, frameRate.maxVal, width, height);
    return AV_ERR_OK;
}

bool OH_AVCapability_AreVideoSizeAndFrameRateSupported(OH_AVCapability *capability, int32_t width, int32_t height,
                                                       int32_t frameRate)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        false, "Invalid parameter");
    CapabilityData *capData = capability->capabilityData_;
    if (!AVCodecInfo::isVideo(capData->codecType)) {
        AVCODEC_LOGW("The capability provided is not expected, should be the video capability");
    }
    std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capData);
    return videoCap->IsSizeAndRateSupported(width, height, frameRate);
}

bool OH_AVCapability_IsFeatureSupported(OH_AVCapability *capability, OH_AVCapabilityFeature feature)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        false, "Invalid parameter");
    bool isValid = (feature >= VIDEO_ENCODER_TEMPORAL_SCALABILITY && feature <= VIDEO_LOW_LATENCY) ||
        feature == VIDEO_ENCODER_B_FRAME;
    CHECK_AND_RETURN_RET_LOG(isValid, false, "Varified feature failed: feature %{public}d is invalid", feature);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capability->capabilityData_);
    return codecInfo->IsFeatureSupported(static_cast<AVCapabilityFeature>(feature));
}

OH_AVFormat *OH_AVCapability_GetFeatureProperties(OH_AVCapability *capability, OH_AVCapabilityFeature feature)
{
    CHECK_AND_RETURN_RET_LOG(capability != nullptr && capability->magic_ == AVMagic::AVCODEC_MAGIC_AVCAPABILITY,
        nullptr, "Invalid parameter");
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capability->capabilityData_);
    Format format;
    if (codecInfo->GetFeatureProperties(static_cast<AVCapabilityFeature>(feature), format) != AVCS_ERR_OK) {
        return nullptr;
    }
    Format::FormatDataMap formatMap = format.GetFormatMap();
    if (formatMap.size() == 0) {
        AVCODEC_LOGW("Get feature properties successfully, but feature %{public}d does not have a property", feature);
        return nullptr;
    }
    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Get feature properties failed: create OH_AVFormat failed");
    avFormat->format_ = format;
    return avFormat;
}