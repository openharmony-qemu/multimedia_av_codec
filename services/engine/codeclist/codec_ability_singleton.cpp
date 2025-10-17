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
            while ((*iter).profileLevelsMap.size() > MAX_MAP_SIZE) {
                auto rIter = (*iter).profileLevelsMap.end();
                (*iter).profileLevelsMap.erase(--rIter);
            }
            std::vector<int32_t> newProfiles;
            (*iter).profiles.swap(newProfiles);
            auto nIter = (*iter).profileLevelsMap.begin();
            while (nIter != (*iter).profileLevelsMap.end()) {
                (*iter).profiles.emplace_back(nIter->first);
                nIter++;
            }
        }
        while ((*iter).measuredFrameRate.size() > MAX_MAP_SIZE) {
            auto rIter = (*iter).measuredFrameRate.end();
            (*iter).measuredFrameRate.erase(--rIter);
        }
        capabilityDataArray_.emplace_back(*iter);
        mimeCapIdxMap_.at(mimeType).emplace_back(beginIdx);
        nameCodecTypeMap_.insert(std::make_pair((*iter).codecName, codecType));
        beginIdx++;
    }
    AVCODEC_LOGD("Register capability successful");
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
} // namespace MediaAVCodec
} // namespace OHOS
