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

#ifndef MEDIA_AVCODEC_DECODING_BEHAVIOR_ANALYZER_H
#define MEDIA_AVCODEC_DECODING_BEHAVIOR_ANALYZER_H

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <vector>

namespace OHOS {
namespace MediaAVCodec {

// 解码行为类型
enum class DecodingBehaviorType {
    UNKNOWN,           // 初始未确定状态
    UNIFORM_SPEED,     // 稳定倍速播放
    NON_UNIFORM_SPEED  // 变速、超高速、切换中等场景
};

// 解码行为快照，用于统计一段时间内的统计信息
struct DecodingBehaviorSnapshot {
    DecodingBehaviorType type;      // 该行为类型
    size_t decSpeedIdx;             // 解码速度索引
    uint32_t duration;              // 该行为的持续时间
};

struct SpeedStats {
    uint32_t duration = 0u;
    uint32_t count = 0u;
};

union SpeedStatsUnion {
    SpeedStats stats;        // 用于统计累加的视图
    uint64_t combinedKey;    // 用于上报的组合键视图
    SpeedStatsUnion() : stats{0u, 0u} {}
};

using DecInfoCallBack = std::function<void(double srcFps, double speed)>;

class DecodingBehaviorAnalyzer {
public:
    explicit DecodingBehaviorAnalyzer(int32_t instanceId);
    void OnFrameConsumed(int64_t pts, bool render);
    void OnFrameDropped();
    void OnStopped(bool isDecEnd);
    void OnChecked(double decFps);
    void SetDecInfoCallBack(DecInfoCallBack cb);

private:
    static constexpr size_t stdSpeedLevels = 8;
    static constexpr std::array<double, stdSpeedLevels> standardSpeeds = {0.0, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};
    std::mutex mutex_;
    uint32_t frameCnt_ = 0;
    uint32_t renderCnt_ = 0;
    uint32_t droppedCnt_ = 0;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastCheckTime_;
    size_t maxPossibleDecSpeedIdx_ = 0;
    DecodingBehaviorType type_ = DecodingBehaviorType::UNKNOWN;
    std::deque<size_t> speedWindow_;
    std::array<size_t, stdSpeedLevels> speedCount_ = {};
    std::vector<DecodingBehaviorSnapshot> history_;
    DecInfoCallBack decInfoCallBack_ = nullptr;
    std::unordered_map<std::string, std::string> eventData_;
    double lastReportedFps_ = -1.0;
    double lastReportedRenderFps_ = -1.0;
    int64_t lastPtsMs_ = -1;
    std::chrono::steady_clock::time_point startSysTime_;
    std::chrono::steady_clock::time_point endSysTime_;
    std::deque<int64_t> ptsWin_;
    double decInSpeed_ = -1.0;
    double srcFps_ = 0.0;
    int32_t instanceId_ = -1;
#ifdef SUPPORT_FPS_FOR_MEMC
    int64_t lastOutPts_[3] = {0};
    size_t lastIndex_ = 0;
    std::pair<int32_t, int32_t> fixedInfo_{};
    std::pair<int32_t, int32_t> curtInfo_{};
    bool isSingleSpeed_{true};
    std::chrono::steady_clock::time_point lastConsumTime_;
#endif
    bool hasSendPauseEvent_{false};
    void ResetIntervalStats();
    double GetSrcFps();
    size_t MatchAndUpdateSpeedStats(double decSpeed);
    DecodingBehaviorType DetermineDecodingBehaviorType();
    bool IsDecChanged(DecodingBehaviorType oldType, size_t oldSpeedIdx,
        DecodingBehaviorType newType, size_t newSpeedIdx);
    void SaveDecodingBehaviorAsSnapshot(
        DecodingBehaviorType type, std::chrono::steady_clock::time_point endTime, size_t decSpeedIdx);
    void ReportSpeedDecodingInfo();
    size_t UpdateSpeedEstimates(double speed);
    void ReportFpsIfSignificantChange(double decFps, double renderFps, double srcFps);
    void HandleBehaviorTransition(DecodingBehaviorType currentType, size_t oldSpeedIdx,
        DecodingBehaviorType newType, size_t newSpeedIdx);
#ifdef SUPPORT_FPS_FOR_MEMC
    bool IsStableFps(float fps);
    void CalcOutputVideoFps(int64_t currentPts, bool render);
    void IsNeedReport();
#endif
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_DECODING_BEHAVIOR_ANALYZER_H