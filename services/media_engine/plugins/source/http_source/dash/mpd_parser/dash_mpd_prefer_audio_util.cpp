/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "DashMpdParser"

#include "dash_mpd_prefer_audio_util.h"
#include "dash_mpd_def.h"
#include "common/log.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

// 检查是否天空音
int32_t CheckRepresentationAudioMode(const DashRepresentationInfo *representation)
{
    if (representation->commonAttrsAndElements_.audioChannelConfigurationList_.size() > 0) {
        DashDescriptor *configuration =
            representation->commonAttrsAndElements_.audioChannelConfigurationList_.front();
        if (configuration != nullptr) {
            static const std::string schemeIdUrl = "urn:mpeg:dash:outputChannelPositionList:2012";
            static const std::string value = "2 0 1 3 17 18"; // 天空音匹配value值
            if (configuration->value_ == value && configuration->schemeIdUrl_ == schemeIdUrl) {
                return 1;
            }
        }
    }

    return 0;
}

// 检查是否音量增强
int32_t CheckRepresentationVolumeAdjust(const DashRepresentationInfo *representation)
{
    if (representation->volumeAdjust_.compare("up") == 0) {
        return 1;
    }
    return 0;
}

// 检查是否是AC3音频
int32_t CheckRepresentationDirectOutput(const DashRepresentationInfo *representation)
{
    if (representation->commonAttrsAndElements_.codecs_.compare("ac-3") == 0) {
        return 1;
    }
    return 0;
}

// 从AdaptationSet中筛选匹配对最高的音频Representation
DashRepresentationInfo *GetPreferAudioRepFromAdptSet(const std::string &mineType, const PreferAudioFilter &filter,
                                                     const std::list<DashRepresentationInfo *> &repList,
                                                     uint32_t &repIndex)
{
    // 传入的Representation不应该为空
    if (repList.empty()) {
        MEDIA_LOG_E("Representation list is empty");
        return nullptr;
    }

    int32_t highestScore = 0;
    uint32_t indexCnt = 0;
    DashRepresentationInfo *highestScoreRep = nullptr;
    for (DashRepresentationInfo *representation : repList) {
        uint32_t indexCntTmp = indexCnt++;
        if (representation == nullptr) {
            continue;
        }

        // 非音频Representation不处理
        std::string repMineType = representation->commonAttrsAndElements_.mimeType_;
        if (repMineType.empty()) {
            repMineType = mineType;
        }

        if (repMineType.empty() || repMineType.find("audio") == std::string::npos) {
            continue;
        }

        int32_t matchScore = GetAudioRepresentationPreferScore(filter, representation);
        // 根据偏好设置与音轨的匹配度，选择分数最高的音轨
        if (highestScoreRep == nullptr || matchScore > highestScore) {
            highestScore = matchScore;
            highestScoreRep = representation;
            repIndex = indexCntTmp;
        }
    }

    MEDIA_LOG_D("Audio representation [" PUBLIC_LOG_S "] is chosen, score " PUBLIC_LOG_D32 ", index:" PUBLIC_LOG_D32,
        ((highestScoreRep != nullptr) ? highestScoreRep->id_.c_str() : "null"), highestScore, repIndex);
    return highestScoreRep;
}

int32_t GetAudioRepresentationPreferScore(const PreferAudioFilter &filter, const DashRepresentationInfo *representation)
{
    // 传入的Representation不应该为空
    if (representation == nullptr) {
        MEDIA_LOG_E("Representation is null");
        // 出错的情况下返回小于的分数值，避免与未设置偏好，或者未匹配的情况冲突
        return -1;
    }

    int32_t matchScore = 0;

    // AC3偏好设置与音轨匹配，匹配权重增加;或者未设置AC3偏好，非AC3音轨匹配权重增加
    if (filter.directOutputType_ == CheckRepresentationDirectOutput(representation)) {
        matchScore += static_cast<int32_t>(PreferAudioWeight::AC3_WEIGHT);
    }

    // 天空音偏好设置与音轨匹配，匹配权重增加;或者未设置天空音偏好，非天空音音轨匹配权重增加
    if (filter.audioChannelMode_ == CheckRepresentationAudioMode(representation)) {
        matchScore += static_cast<int32_t>(PreferAudioWeight::AUDIO_CHANNEL_MODE_WEIGHT);
    }

    // 音量增强偏好设置与音轨匹配，匹配权重增加;或者未设置音量增强偏好，非音量增强音轨匹配权重增加
    if (filter.volumeAdjust_ == CheckRepresentationVolumeAdjust(representation)) {
        matchScore += static_cast<int32_t>(PreferAudioWeight::VOLUME_ADJUST_WEIGHT);
    }

    MEDIA_LOG_D("Audio representation [" PUBLIC_LOG_S "] prefer score is "
        PUBLIC_LOG_D32, representation->id_.c_str(), matchScore);
    return matchScore;
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
