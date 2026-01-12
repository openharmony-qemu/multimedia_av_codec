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

#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "codeclist_builder.h"
#ifndef CLIENT_SUPPORT_CODEC
#include "hcodec_loader.h"
#endif
#include "codec_ability_singleton.h"
#include "instance_info.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecAbilitySingleton"};
constexpr int32_t MAX_SQR_FACTOR = 51;
}

namespace OHOS {
namespace MediaAVCodec {
std::unordered_map<CodecType, std::shared_ptr<CodecListBase>> GetCodecLists()
{
    std::unordered_map<CodecType, std::shared_ptr<CodecListBase>> codecLists;
#ifndef CLIENT_SUPPORT_CODEC
    std::shared_ptr<CodecListBase> vcodecList = std::make_shared<VideoCodecList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_VIDEO_CODEC, vcodecList));
    std::shared_ptr<CodecListBase> hevcDecoderList = std::make_shared<VideoHevcDecoderList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_VIDEO_HEVC_DECODER, hevcDecoderList));
#ifdef SUPPORT_CODEC_VP8
    std::shared_ptr<CodecListBase> vp8DecoderList = std::make_shared<VideoVp8DecoderList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_VIDEO_VP8_DECODER, vp8DecoderList));
#endif
#ifdef SUPPORT_CODEC_VP9
    std::shared_ptr<CodecListBase> vp9DecoderList = std::make_shared<VideoVp9DecoderList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_VIDEO_VP9_DECODER, vp9DecoderList));
#endif
#ifdef SUPPORT_CODEC_AV1
    std::shared_ptr<CodecListBase> av1DecoderList = std::make_shared<VideoAv1DecoderList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_VIDEO_AV1_DECODER, av1DecoderList));
#endif
    std::shared_ptr<CodecListBase> avcEncoderList = std::make_shared<VideoAvcEncoderList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_VIDEO_AVC_ENCODER, avcEncoderList));
#endif
    std::shared_ptr<CodecListBase> acodecList = std::make_shared<AudioCodecList>();
    codecLists.insert(std::make_pair(CodecType::AVCODEC_AUDIO_CODEC, acodecList));
    return codecLists;
}

CodecAbilitySingleton &CodecAbilitySingleton::GetInstance()
{
    static CodecAbilitySingleton instance;
    return instance;
}

CodecAbilitySingleton::CodecAbilitySingleton()
{
#ifndef CLIENT_SUPPORT_CODEC
    std::vector<CapabilityData> videoCapaArray;
    if (HCodecLoader::GetCapabilityList(videoCapaArray) == AVCS_ERR_OK) {
        RegisterCapabilityArray(videoCapaArray, CodecType::AVCODEC_HCODEC);
    }
#endif
    std::unordered_map<CodecType, std::shared_ptr<CodecListBase>> codecLists = GetCodecLists();
    for (auto iter = codecLists.begin(); iter != codecLists.end(); iter++) {
        CodecType codecType = iter->first;
        std::vector<CapabilityData> capaArray;
        int32_t ret = iter->second->GetCapabilityList(capaArray);
        if (ret == AVCS_ERR_OK) {
            RegisterCapabilityArray(capaArray, codecType);
        }
    }
}

CodecAbilitySingleton::~CodecAbilitySingleton()
{
    AVCODEC_LOGI("Succeed");
}

bool CodecAbilitySingleton::IsCapabilityValid(const CapabilityData &cap)
{
    CHECK_AND_RETURN_RET_LOGW(!cap.codecName.empty(), false, "codecName is empty");
    CHECK_AND_RETURN_RET_LOG(cap.codecType > AVCodecType::AVCODEC_TYPE_NONE && cap.codecType <=
        AVCodecType::AVCODEC_TYPE_AUDIO_DECODER, false, "codecType is invalid, %{public}d", cap.codecType);
    CHECK_AND_RETURN_RET_LOG(!cap.mimeType.empty(), false, "mimeType is empty");
    CHECK_AND_RETURN_RET_LOG(cap.maxInstance > 0, false, "maxInstance is invalid, %{public}d", cap.maxInstance);
    if (cap.codecType == AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER ||
        cap.codecType == AVCodecType::AVCODEC_TYPE_VIDEO_DECODER) {
        CHECK_AND_RETURN_RET_LOG(cap.width.minVal > 0 && cap.width.minVal <= cap.width.maxVal, false,
            "width is invalid, range is [%{public}d, %{public}d]", cap.width.minVal, cap.width.maxVal);
        CHECK_AND_RETURN_RET_LOG(cap.height.minVal > 0 && cap.height.minVal <= cap.height.maxVal, false,
            "height is invalid, range is [%{public}d, %{public}d]", cap.height.minVal, cap.height.maxVal);
    }
    if (std::find(cap.bitrateMode.begin(), cap.bitrateMode.end(), SQR) != cap.bitrateMode.end()) {
        CHECK_AND_RETURN_RET_LOG(cap.sqrFactor.maxVal <= MAX_SQR_FACTOR && cap.sqrFactor.minVal >= 0, false,
            "sqrFactor is invalid, range is [%{public}d, %{public}d]", cap.sqrFactor.minVal, cap.sqrFactor.maxVal);
    }
    return true;
}

void CodecAbilitySingleton::RegisterCapabilityArray(std::vector<CapabilityData> &capaArray, CodecType codecType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t beginIdx = capabilityDataArray_.size();
    for (auto iter = capaArray.begin(); iter != capaArray.end(); iter++) {
        if (!IsCapabilityValid(*iter)) {
            if (!(*iter).codecName.empty()) {
                AVCODEC_LOGE("cap is invalid, codecName is %{public}s", (*iter).codecName.c_str());
            }
            continue;
        }
        std::string mimeType = (*iter).mimeType;
        std::vector<size_t> idxVec;
        if (mimeCapIdxMap_.find(mimeType) == mimeCapIdxMap_.end()) {
            mimeCapIdxMap_.insert(std::make_pair(mimeType, idxVec));
        }
        if ((*iter).profileLevelsMap.size() > MAX_MAP_SIZE) {
            AVCODEC_LOGW("current profileLevelsMap size: %{public}zu, codecName is %{public}s",
                (*iter).profileLevelsMap.size(), (*iter).codecName.c_str());
            std::map<int32_t, std::vector<int32_t>> oldProfileLevelsMap = (*iter).profileLevelsMap;
            std::map<int32_t, std::vector<int32_t>> newProfileLevelsMap;
            auto it = oldProfileLevelsMap.begin();
            for (uint32_t i = 0u; i < MAX_MAP_SIZE && it != oldProfileLevelsMap.end(); ++i, ++it) {
                newProfileLevelsMap.insert(*it);
            }
            (*iter).profileLevelsMap = newProfileLevelsMap;
            std::vector<int32_t> newProfiles;
            auto nIter = newProfileLevelsMap.begin();
            while (nIter != newProfileLevelsMap.end()) {
                newProfiles.emplace_back(nIter->first);
                nIter++;
            }
            (*iter).profiles.swap(newProfiles);
        }
        if ((*iter).measuredFrameRate.size() > MAX_MAP_SIZE) {
            AVCODEC_LOGW("current measuredFrameRate map size: %{public}zu, codecName is %{public}s",
                (*iter).measuredFrameRate.size(), (*iter).codecName.c_str());
            std::map<ImgSize, Range> oldMeasuredFrameRate = (*iter).measuredFrameRate;
            std::map<ImgSize, Range> newMeasuredFrameRate;
            auto it = oldMeasuredFrameRate.begin();
            for (uint32_t i = 0u; i < MAX_MAP_SIZE && it != oldMeasuredFrameRate.end(); ++i, ++it) {
                newMeasuredFrameRate.insert(*it);
            }
            (*iter).measuredFrameRate = newMeasuredFrameRate;
        }
        capabilityDataArray_.emplace_back(*iter);
        mimeCapIdxMap_.at(mimeType).emplace_back(beginIdx);
        nameCodecTypeMap_.insert(std::make_pair((*iter).codecName, codecType));
        beginIdx++;
    }
}

std::vector<CapabilityData> CodecAbilitySingleton::GetCapabilityArray()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return capabilityDataArray_;
}

std::optional<CapabilityData> CodecAbilitySingleton::GetCapabilityByName(const std::string &name)
{
    auto it = std::find_if(capabilityDataArray_.begin(), capabilityDataArray_.end(), [&](const CapabilityData &cap) {
        return cap.codecName == name;
    });
    return it == capabilityDataArray_.end() ? std::nullopt : std::make_optional<CapabilityData>(*it);
}

std::string CodecAbilitySingleton::GetMimeByCodecName(const std::string & name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &cap : capabilityDataArray_) {
        if (cap.codecName == name) {
            return cap.mimeType;
        }
    }
    return "";
}

std::unordered_map<std::string, CodecType> CodecAbilitySingleton::GetNameCodecTypeMap()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return nameCodecTypeMap_;
}

std::unordered_map<std::string, std::vector<size_t>> CodecAbilitySingleton::GetMimeCapIdxMap()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return mimeCapIdxMap_;
}

int32_t CodecAbilitySingleton::GetVideoCodecTypeByCodecName(const std::string &codecName)
{
    constexpr auto hdecPair = std::pair(true,  static_cast<int32_t>(AVCODEC_TYPE_VIDEO_DECODER));
    constexpr auto sdecPair = std::pair(false, static_cast<int32_t>(AVCODEC_TYPE_VIDEO_DECODER));
    constexpr auto hencPair = std::pair(true,  static_cast<int32_t>(AVCODEC_TYPE_VIDEO_ENCODER));
    constexpr auto sencPair = std::pair(false, static_cast<int32_t>(AVCODEC_TYPE_VIDEO_ENCODER));

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(capabilityDataArray_.begin(), capabilityDataArray_.end(), [&](const CapabilityData &cap) {
        return cap.codecName == codecName;
    });
    if (it == capabilityDataArray_.end()) {
        return static_cast<int32_t>(VideoCodecType::UNKNOWN);
    }

    int32_t ret = static_cast<int32_t>(VideoCodecType::UNKNOWN);
    auto vcodecTypePair = std::make_pair(it->isVendor, it->codecType);
    if (vcodecTypePair == hdecPair) {
        ret = static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE);
    } else if (vcodecTypePair == hencPair) {
        ret = static_cast<int32_t>(VideoCodecType::ENCODER_HARDWARE);
    } else if (vcodecTypePair == sdecPair) {
        ret = static_cast<int32_t>(VideoCodecType::DECODER_SOFTWARE);
    } else if (vcodecTypePair == sencPair) {
        ret = static_cast<int32_t>(VideoCodecType::ENCODER_SOFTWARE);
    }
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS
