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

#ifndef DASH_MPD_PREFER_AUDIO_UTIL_H
#define DASH_MPD_PREFER_AUDIO_UTIL_H

#include <stdint.h>
#include <string>
#include <list>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
// 音频的筛选条件权重值
enum class PreferAudioWeight { VOLUME_ADJUST_WEIGHT = 1, AUDIO_CHANNEL_MODE_WEIGHT = 10, AC3_WEIGHT = 100 };

// 首选音频的筛选条件
struct PreferAudioFilter {
    // ac3音轨偏好
    int32_t directOutputType_;
    // 天空音音效偏好
    int32_t audioChannelMode_;
    // 音量增强偏好
    int32_t volumeAdjust_;

    PreferAudioFilter()
    {
        directOutputType_ = 0;
        audioChannelMode_ = 0;
        volumeAdjust_ = 0;
    }
};

struct DashRepresentationInfo;

// 检查是否天空音
int32_t CheckRepresentationAudioMode(const DashRepresentationInfo *representation);

// 检查是否音量增强
int32_t CheckRepresentationVolumeAdjust(const DashRepresentationInfo *representation);

// 检查是否是AC3音频
int32_t CheckRepresentationDirectOutput(const DashRepresentationInfo *representation);

// 从AdaptationSet中筛选匹配度最高的音频Representation
DashRepresentationInfo *GetPreferAudioRepFromAdptSet(const std::string &mineType, const PreferAudioFilter &filter,
                                                     const std::list<DashRepresentationInfo *> &repList,
                                                     uint32_t &repIndex);

// 获取单个音频Representation的匹配度
int32_t GetAudioRepresentationPreferScore(const PreferAudioFilter &filter,
                                          const DashRepresentationInfo *representation);

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif
