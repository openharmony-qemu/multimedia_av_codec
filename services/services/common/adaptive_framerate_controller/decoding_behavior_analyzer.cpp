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

#include "decoding_behavior_analyzer.h"
#include "avcodec_log_ex.h"
#include "event_manager.h"
#include "event_info_extented_key.h"
#include <map>
#include <mutex>

namespace {
using namespace std::chrono;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "DBA"};
constexpr size_t SPEED_WINDON_SIZE = 5;           // Sliding window size used to determine speed stability
constexpr double DETECTION_THRESHOLD_RATIO = 0.5; // Stability threshold: dominant speed in the window must exceed 50%
constexpr uint32_t SRC_FRAME_CNT_MIN = 5;         // Minimum valid frame count
const std::unordered_map<size_t, const std::string_view> SPEED_DEC_INFO_KEY_MAP = {
    {1, OHOS::MediaAVCodec::EventInfoExtentedKey::SPEED_DECODING_INFO_0_75X},
    {2, OHOS::MediaAVCodec::EventInfoExtentedKey::SPEED_DECODING_INFO_1_00X},
    {3, OHOS::MediaAVCodec::EventInfoExtentedKey::SPEED_DECODING_INFO_1_25X},
    {4, OHOS::MediaAVCodec::EventInfoExtentedKey::SPEED_DECODING_INFO_1_50X},
    {5, OHOS::MediaAVCodec::EventInfoExtentedKey::SPEED_DECODING_INFO_2_00X},
    {6, OHOS::MediaAVCodec::EventInfoExtentedKey::SPEED_DECODING_INFO_3_00X}
};
}

namespace OHOS {
namespace MediaAVCodec {
DecodingBehaviorAnalyzer::DecodingBehaviorAnalyzer()
{
    startTime_ = std::chrono::steady_clock::now();
}

void DecodingBehaviorAnalyzer::OnFrameConsumed(int64_t pts)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (pts < 0) {
        return;
    }
    int64_t ptsMs = pts / 1000; // 1000: us to ms
    if (ptsMs < startPtsMs_) { // handle looped playback
        frameCnt_ = 0;
    }
    if (frameCnt_ == 0) {
        startPtsMs_ = ptsMs;
    }
    endPtsMs_ = ptsMs;
    frameCnt_++;
}

void DecodingBehaviorAnalyzer::SaveDecodingBehaviorAsSnapshot(
    DecodingBehaviorType type, std::chrono::steady_clock::time_point endTime, size_t decSpeedIdx)
{
    DecodingBehaviorSnapshot snapshot;
    snapshot.type = type;
    snapshot.decSpeedIdx = decSpeedIdx;
    snapshot.duration = static_cast<uint32_t>(duration_cast<milliseconds>(endTime - startTime_).count());
    history_.push_back(snapshot);
    startTime_ = endTime;
    AVCODEC_LOGD("snapshot: %{public}d-%{public}.2fx-%{public}u, history size %{public}zu ",
        snapshot.type, STANDARD_SPEEDS[snapshot.decSpeedIdx], snapshot.duration, history_.size());
}

void DecodingBehaviorAnalyzer::ReportSpeedDecodingInfo()
{
    SpeedStatsUnion totalStats;
    std::array<SpeedStatsUnion, stdSpeedLevels> speedStatsArray = {};
    for (const auto& snapshot : history_) {
        totalStats.stats.duration += snapshot.duration;
        totalStats.stats.count++;
        if (snapshot.type != DecodingBehaviorType::UNIFORM_SPEED) {
            continue;
        }
        if (snapshot.decSpeedIdx < stdSpeedLevels) {
            speedStatsArray[snapshot.decSpeedIdx].stats.duration += snapshot.duration;
            speedStatsArray[snapshot.decSpeedIdx].stats.count++;
        }
    }
    Media::Meta eventMeta;
    eventMeta.SetData(EventInfoExtentedKey::SPEED_DECODING_INFO_TOTAL.data(), totalStats.combinedKey);
    for (size_t i = 0; i < stdSpeedLevels; ++i) {
        if (speedStatsArray[i].stats.count > 0) {
            auto iter = SPEED_DEC_INFO_KEY_MAP.find(i);
            if (iter != SPEED_DEC_INFO_KEY_MAP.end()) {
                eventMeta.SetData(iter->second.data(), speedStatsArray[i].combinedKey);
            }
        }
    }
    EventManager::GetInstance().OnInstanceEvent(StatisticsEventType::SPEED_DECODING_INFO, eventMeta);
    history_.clear();
}

void DecodingBehaviorAnalyzer::OnStopped(bool isDecEnd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isDecEnd) { // ignore triggers from flush
        return;
    }
    if (lastCheckTime_ != std::chrono::steady_clock::time_point{}) {
        // Use lastCheckTime_ to base on a valid statistics interval and avoid miscount during long pauses
        SaveDecodingBehaviorAsSnapshot(type_, lastCheckTime_, maxPossibleDecSpeedIdx_);
    }
    ReportSpeedDecodingInfo();
    frameCnt_ = 0;
    type_ = DecodingBehaviorType::UNKNOWN;
    speedWindow_.clear();
    for (auto& element : speedCount_) {
        element = 0;
    }
}

size_t DecodingBehaviorAnalyzer::MatchAndUpdateSpeedStats(double decSpeed)
{
    size_t closestIndex = 0;
    double minDiff = std::abs(decSpeed - STANDARD_SPEEDS[0]);
    for (size_t i = 1; i < stdSpeedLevels; ++i) {
        double diff = std::abs(decSpeed - STANDARD_SPEEDS[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closestIndex = i;
        }
    }

    if (speedWindow_.size() >= SPEED_WINDON_SIZE) {
        size_t oldestIndex = speedWindow_.front();
        speedCount_[oldestIndex]--;
        speedWindow_.pop_front();
    }
    speedWindow_.push_back(closestIndex);
    speedCount_[closestIndex]++;

    return closestIndex;
}

DecodingBehaviorType DecodingBehaviorAnalyzer::DetermineDecodingBehaviorType()
{
    if (speedWindow_.size() < SPEED_WINDON_SIZE) {
        return DecodingBehaviorType::UNKNOWN;
    }
    size_t maxCnt = 0;
    size_t maxCntIdx = 0;
    for (size_t i = 0; i < speedCount_.size(); ++i) {
        if (speedCount_[i] > maxCnt) {
            maxCnt = speedCount_[i];
            maxCntIdx = i;
        }
    }
    maxPossibleDecSpeedIdx_ = maxCntIdx;
    bool isStable = static_cast<double>(maxCnt) / SPEED_WINDON_SIZE > DETECTION_THRESHOLD_RATIO;
    // 0.0 and 4.0 are placeholders for extremely slow and extremely fast speeds respectively
    bool isUniformSpeed = (maxCntIdx != 0u) && (maxCntIdx != stdSpeedLevels - 1);
    AVCODEC_LOGD("MaxCnt %{public}zu, maxPSpeed %{public}.2fx, isStable %{public}d, isUniformSpeed %{public}d",
        maxCnt, STANDARD_SPEEDS[maxCntIdx], isStable, isUniformSpeed);
    if (isStable && isUniformSpeed) {
        return DecodingBehaviorType::UNIFORM_SPEED;
    }
    return DecodingBehaviorType::NON_UNIFORM_SPEED;
}

bool DecodingBehaviorAnalyzer::IsDecChanged(DecodingBehaviorType oldType, size_t oldSpeedIdx,
                                            DecodingBehaviorType newType, size_t newSpeedIdx)
{
    if (oldType == DecodingBehaviorType::UNKNOWN) {
        return false; // not yet stable
    } else if (oldType == DecodingBehaviorType::NON_UNIFORM_SPEED) {
        return newType != oldType; // only respond to behavior mode changes
    } else { // DecodingBehaviorType::UNIFORM_SPEED
        return newType != oldType || oldSpeedIdx != newSpeedIdx;
    }
}

void DecodingBehaviorAnalyzer::OnChecked(double decFps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameCnt_ < SRC_FRAME_CNT_MIN || endPtsMs_ <= startPtsMs_) {
        return;
    }
    double ptsIntvlMs = static_cast<double>(endPtsMs_ - startPtsMs_) / (frameCnt_ - 1);
    double srcFps = 1000.0 / ptsIntvlMs;
    AVCODEC_LOGD("%{public}" PRId64 ", %{public}" PRId64 ", %{public}u", endPtsMs_, startPtsMs_, frameCnt_);
    startPtsMs_ = -1;
    endPtsMs_ = -1;
    frameCnt_ = 0;

    double decSpeed = decFps / srcFps;
    size_t newSpeedIdx = MatchAndUpdateSpeedStats(decSpeed);
    size_t lastDecSpeedIdx = maxPossibleDecSpeedIdx_;
    DecodingBehaviorType newType = DetermineDecodingBehaviorType();
    if (IsDecChanged(type_, lastDecSpeedIdx, newType, newSpeedIdx)) {
        std::chrono::steady_clock::time_point endTime = lastCheckTime_ != std::chrono::steady_clock::time_point{} ?
            lastCheckTime_ : std::chrono::steady_clock::now();
        AVCODEC_LOGD("type %{public}d, speed %{public}.2fx, newType %{public}d, new speed %{public}.2fx,",
            type_, STANDARD_SPEEDS[lastDecSpeedIdx], newType, STANDARD_SPEEDS[newSpeedIdx]);
        SaveDecodingBehaviorAsSnapshot(type_, endTime, lastDecSpeedIdx);
    }
    type_ = newType;
    lastCheckTime_ = std::chrono::steady_clock::now();
}
} // namespace MediaAVCodec
} // namespace OHOS