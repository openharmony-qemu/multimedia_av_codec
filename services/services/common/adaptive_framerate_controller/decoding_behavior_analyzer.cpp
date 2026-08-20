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
#include "avcodec_server_manager.h"
#include "connect_adapter/rs_interface_adapter.h"
#include <map>
#include <mutex>

namespace {
using namespace std::chrono;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "DBA"};
constexpr size_t SPEED_WINDOW_SIZE = 5;           // Sliding window size used to determine speed stability
constexpr size_t PTS_WINDOW_MIN_SIZE = 2;
constexpr size_t PTS_WINDOW_MAX_SIZE = 30;
constexpr double DETECTION_THRESHOLD_RATIO = 0.5; // Stability threshold: dominant speed in the window must exceed 50%
constexpr uint32_t SRC_FRAME_CNT_MIN = 5;         // Minimum valid frame count
constexpr double EPSILON = 1e-6;
constexpr double FPS_REPORT_DIFF_THRESHOLD = 2.0 + EPSILON;
constexpr double MAX_REPORT_FPS = 60.0;
constexpr int64_t SEEK_DETECTION_THRESHOLD_MS = 1000;
constexpr double MIN_CAL_DURATION_MS = 100.0;
constexpr double SMOOTHING_FACTOR = 0.5;
constexpr int64_t MIN_REASONABLE_DIFF_US = 8000;
#ifdef SUPPORT_FPS_FOR_MEMC
constexpr size_t FPS_WINDOW_SIZE = 3;
constexpr uint32_t CONTINUES_FRAME_CNT = 5;
constexpr int64_t PTS_TIME_UNIT = 1000000;
constexpr int64_t MAX_FPS = 60;
constexpr int64_t FPS_THRESHOLD = 1000;
constexpr uint32_t NORMAL_SPEED = 2;
constexpr uint32_t FLOAT2INT = 100;
#endif
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
DecodingBehaviorAnalyzer::DecodingBehaviorAnalyzer(int32_t instanceId)
{
    startTime_ = std::chrono::steady_clock::now();
    instanceId_ = instanceId;
#ifndef SUPPORT_FPS_FOR_MEMC
    eventData_.insert(std::make_pair("decRate", "0"));
#endif
#ifdef SUPPORT_FPS_FOR_MEMC
    tveventData_.insert(std::make_pair("rate", "0"));
    tveventData_.insert(std::make_pair("decSpeed", "100"));
#endif
}

void DecodingBehaviorAnalyzer::OnFrameConsumed(int64_t pts, bool render)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (pts < 0) {
        return;
    }
#ifdef SUPPORT_FPS_FOR_MEMC
    CalcOutputVideoFps(pts, render);
#endif
    int64_t ptsMs = pts / 1000; // 1000: us to ms
    if (lastPtsMs_ != -1 && std::abs(ptsMs - lastPtsMs_) > SEEK_DETECTION_THRESHOLD_MS) {
        AVCODEC_LOGD("seek detection, lastPtsMs %{public}" PRId64 ", ptsMs %{public}" PRId64 "", lastPtsMs_, ptsMs);
        ResetIntervalStats();
    }
    if (frameCnt_ == 0) {
        startSysTime_ = std::chrono::steady_clock::now();
    }
    ptsWin_.push_back(pts);
    if (ptsWin_.size() > PTS_WINDOW_MAX_SIZE) {
        ptsWin_.pop_front();
    }
    endSysTime_ = std::chrono::steady_clock::now();
    frameCnt_++;
    if (render) {
        renderCnt_++;
    }
    lastPtsMs_ = ptsMs;
    AVCODEC_LOGD("frameCnt_ %{public}u, renderCnt_ %{public}u, pts= %{public}" PRId64 "",
        frameCnt_, renderCnt_, pts);
}

double DecodingBehaviorAnalyzer::GetSrcFps()
{
    std::vector<int64_t> sortedPts(ptsWin_.begin(), ptsWin_.end());
    std::sort(sortedPts.begin(), sortedPts.end());
    int64_t minValidDiff = INT64_MAX;
    for (size_t i = 1; i < sortedPts.size(); ++i) {
        int64_t diff = sortedPts[i] - sortedPts[i - 1];
        if (diff >= MIN_REASONABLE_DIFF_US && diff < minValidDiff) {
            minValidDiff = diff;
        }
    }
    if (minValidDiff == 0) {
        return 0.0;
    }
    return 1000000.0 / minValidDiff; // 1000000.0: us to s
}

#ifdef SUPPORT_FPS_FOR_MEMC
bool DecodingBehaviorAnalyzer::IsStableFps(float fps)
{
    constexpr float STABLE_FPS1 = 24.0;
    constexpr float STABLE_FPS2 = 25.0;
    constexpr float STABLE_FPS3 = 30.0;
    constexpr float STABLE_FPS4 = 60.0;
    constexpr float STABLE_FPS5 = 120.0;
    constexpr float STABLE_FPS6 = 50.0;
    if (abs(fps - STABLE_FPS1) < 1 || abs(fps - STABLE_FPS2) < 1 || abs(fps - STABLE_FPS3) < 1 ||
        abs(fps - STABLE_FPS4) < 1 || abs(fps - STABLE_FPS5) < 1 || abs(fps - STABLE_FPS6) < 1) {
        return true;
    }
    return false;
}

void DecodingBehaviorAnalyzer::IsNeedReport()
{
    std::chrono::steady_clock::time_point curt = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(curt - lastConsumTime_).count();
    if (elapsedTime > SEEK_DETECTION_THRESHOLD_MS && lastConsumTime_ != std::chrono::steady_clock::time_point{}
         && fixedInfo_.first != 0) {
        tveventData_["rate"] = std::to_string(fixedInfo_.first);
        RSInterfaceAdapter::GetInstance().NotifyFrameRateEvent(tveventData_);
        AVCODEC_LOGI("video fix fps =%{public}u", fixedInfo_.first);
    }
    lastConsumTime_ = std::chrono::steady_clock::now();
}

void DecodingBehaviorAnalyzer::CalcOutputVideoFps(int64_t currentPts, bool render)
{
    IsNeedReport();
    int64_t lastPts = lastOutPts_[lastIndex_];
    int64_t delta = currentPts - lastPts;
    if (delta > 0) {
        float fps = (static_cast<float>(PTS_TIME_UNIT) / delta) * FPS_WINDOW_SIZE;
        if (fps > FPS_THRESHOLD) {
            fps /= static_cast<float>(FPS_THRESHOLD);
        }
        if (IsStableFps(fps)) {
            if (fps > MAX_FPS) {
                fps = MAX_FPS;
            }
        }
        uint8_t rate = static_cast<uint8_t>(round(fps));
        if (fixedInfo_.first == rate) {
            fixedInfo_.second++;
            curtInfo_.first = 0;
            curtInfo_.second = 0;
        } else {
            if (curtInfo_.first != rate) {
                curtInfo_.first = rate;
                curtInfo_.second = 1;
            } else {
                curtInfo_.second += 1; //连续相同帧率
            }
            if (curtInfo_.second >= CONTINUES_FRAME_CNT && IsStableFps(fps)) {
                fixedInfo_.first = curtInfo_.first; //连续相同帧率超过阈值，记录为稳定帧率
                fixedInfo_.second = curtInfo_.second;
                tveventData_["rate"] = std::to_string(fixedInfo_.first);
                RSInterfaceAdapter::GetInstance().NotifyFrameRateEvent(tveventData_);
                AVCODEC_LOGI("video fix fps =%{public}u pts= %{public}ld, lastPts= %{public}ld, delta= %{public}ld",
                    fixedInfo_.first, currentPts, lastPts, delta);
            }
        }
        if (fixedInfo_.second >= curtInfo_.second) {
            rate = fixedInfo_.first;
        }
        AVCODEC_LOGD("video fps =%{public}u buffer2 pts= %{public}ld, lastPts= %{public}ld, delta= %{public}ld",
            rate, currentPts, lastPts, delta);
    }
    lastOutPts_[lastIndex_] = currentPts;
    lastIndex_ = (lastIndex_ + 1) % FPS_WINDOW_SIZE;
}
#endif

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
        snapshot.type, standardSpeeds[snapshot.decSpeedIdx], snapshot.duration, history_.size());
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
    ResetIntervalStats();
    lastReportedFps_ = -1.0;
    lastReportedRenderFps_ = -1.0;
    type_ = DecodingBehaviorType::UNKNOWN;
    speedWindow_.clear();
    for (auto& element : speedCount_) {
        element = 0;
    }

    eventData_["decRate"] = "0";
    RSInterfaceAdapter::GetInstance().NotifyFrameRateEvent(eventData_);
}

size_t DecodingBehaviorAnalyzer::MatchAndUpdateSpeedStats(double decSpeed)
{
    size_t closestIndex = 0;
    double minDiff = std::abs(decSpeed - standardSpeeds[0]);
    for (size_t i = 1; i < stdSpeedLevels; ++i) {
        double diff = std::abs(decSpeed - standardSpeeds[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closestIndex = i;
        }
    }

    if (speedWindow_.size() >= SPEED_WINDOW_SIZE) {
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
    if (speedWindow_.size() < SPEED_WINDOW_SIZE) {
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
    bool isStable = static_cast<double>(maxCnt) / SPEED_WINDOW_SIZE > DETECTION_THRESHOLD_RATIO;
    // 0.0 and 4.0 are placeholders for extremely slow and extremely fast speeds respectively
    bool isUniformSpeed = (maxCntIdx != 0u) && (maxCntIdx != stdSpeedLevels - 1);
    AVCODEC_LOGD("MaxCnt %{public}zu, maxPSpeed %{public}.2fx, isStable %{public}d, isUniformSpeed %{public}d",
        maxCnt, standardSpeeds[maxCntIdx], isStable, isUniformSpeed);
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

void DecodingBehaviorAnalyzer::OnFrameDropped()
{
    std::lock_guard<std::mutex> lock(mutex_);
    droppedCnt_++;
}

void DecodingBehaviorAnalyzer::ResetIntervalStats()
{
    lastPtsMs_ = -1;
    frameCnt_ = 0;
    renderCnt_ = 0;
    droppedCnt_ = 0;
    ptsWin_.clear();
    hasSendPauseEvent_ = false;
    AVCODEC_LOGD("ResetIntervalStats");
}

void DecodingBehaviorAnalyzer::OnChecked(double decFps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t totalFrames = frameCnt_ + droppedCnt_;
    auto sysTimeIntvlMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endSysTime_ - startSysTime_).count();

    if (totalFrames < SRC_FRAME_CNT_MIN) {
        if (!hasSendPauseEvent_) {
            eventData_["decRate"] = "0";
            RSInterfaceAdapter::GetInstance().NotifyFrameRateEvent(eventData_);
            hasSendPauseEvent_ = true;
        }
        return;
    }

    if (sysTimeIntvlMs <= MIN_CAL_DURATION_MS || ptsWin_.size() < PTS_WINDOW_MIN_SIZE) {
        return;
    }
    double decInFps = static_cast<double>(totalFrames - 1) / sysTimeIntvlMs * 1000; // 1000: ms to s
#ifdef SUPPORT_FPS_FOR_MEMC
    double srcFps = fixedInfo_.first;
    srcFps_ = srcFps;
#else
    double srcFps = GetSrcFps();
#endif
    double renderFps = decFps * static_cast<double>(renderCnt_) / frameCnt_;
    ReportFpsIfSignificantChange(decFps, renderFps, srcFps);
    if (srcFps < 1.0) {
        return;
    }
    if (srcFps > srcFps_ + EPSILON) {
        srcFps_ = srcFps;
    }
    double speed = decInFps / srcFps_;
    size_t newSpeedIdx = UpdateSpeedEstimates(speed);
    size_t lastDecSpeedIdx = maxPossibleDecSpeedIdx_;
#ifdef SUPPORT_FPS_FOR_MEMC
    if ((lastDecSpeedIdx != 0 && lastDecSpeedIdx != NORMAL_SPEED && isSingleSpeed_) ||
        (lastDecSpeedIdx == NORMAL_SPEED && !isSingleSpeed_)) {
        tveventData_["decSpeed"] = std::to_string(static_cast<int>(standardSpeeds[lastDecSpeedIdx] * FLOAT2INT));
        RSInterfaceAdapter::GetInstance().NotifyFrameRateEvent(tveventData_);
        AVCODEC_LOGI("video fix fps =%{public}u, new =%{public}zu last =%{public}zu",
            fixedInfo_.first, newSpeedIdx, lastDecSpeedIdx);
    }
    isSingleSpeed_ = lastDecSpeedIdx == NORMAL_SPEED || lastDecSpeedIdx == 0;
#endif
    DecodingBehaviorType newType = DetermineDecodingBehaviorType();
    HandleBehaviorTransition(type_, lastDecSpeedIdx, newType, newSpeedIdx);
    type_ = newType;
    lastCheckTime_ = std::chrono::steady_clock::now();
    ResetIntervalStats();
}

size_t DecodingBehaviorAnalyzer::UpdateSpeedEstimates(double speed)
{
    if (decInSpeed_ < 0.0) {
        decInSpeed_ = speed;
    } else {
        decInSpeed_ = (1.0 - SMOOTHING_FACTOR) * decInSpeed_ + SMOOTHING_FACTOR * speed;
    }
    size_t newSpeedIdx = MatchAndUpdateSpeedStats(decInSpeed_);
    double stdSpeed = standardSpeeds[newSpeedIdx];
    AVCODEC_LOGD("speed: %{public}.2f, avgSpeed: %{public}.2f, stdSpeed %{public}.2f, srcFps %{public}.2f",
        speed, decInSpeed_, stdSpeed, srcFps_);
    if (decInfoCallBack_) {
        decInfoCallBack_(srcFps_, stdSpeed);
    }
    return newSpeedIdx;
}

void DecodingBehaviorAnalyzer::ReportFpsIfSignificantChange(double decFps, double renderFps, double srcFps)
{
    AVCODEC_LOGD("Render: %{public}.2f fps, dec: %{public}.2f fps, src: %{public}.2f fps, renderCnt %{public}u"
        ", frameCnt %{public}u", renderFps, decFps, srcFps, renderCnt_, frameCnt_);
    double reportDecFps = decFps > (MAX_REPORT_FPS + EPSILON) ? (MAX_REPORT_FPS + 1.0) : decFps;
    double reportRenderFps = renderFps > (MAX_REPORT_FPS + EPSILON) ? (MAX_REPORT_FPS + 1.0) : renderFps;
    if (lastReportedFps_ < 0.0 || lastReportedRenderFps_ < 0.0 ||
        std::abs(reportRenderFps - lastReportedRenderFps_) > FPS_REPORT_DIFF_THRESHOLD ||
        std::abs(reportDecFps - lastReportedFps_) > FPS_REPORT_DIFF_THRESHOLD) {
        AVCODEC_LOGD("Framerate changed: render: %{public}.2f fps, dec: %{public}.2f fps, src: %{public}.2f fps",
                     reportRenderFps, reportDecFps, srcFps);
        lastReportedRenderFps_ = reportRenderFps;
        lastReportedFps_ = reportDecFps;
        if (eventData_.count("pid") == 0) {
            auto instanceInfoOpt = AVCodecServerManager::GetInstance().GetInstanceInfoByInstanceId(instanceId_);
            CHECK_AND_RETURN_LOG(instanceInfoOpt != std::nullopt,
                "Can not find this instance, id:%{public}d", instanceId_);
            auto instanceInfo = instanceInfoOpt.value();
            if (instanceInfo.forwardCaller.pid == -1) {
                eventData_["pid"] = std::to_string(instanceInfo.caller.pid);
            } else {
                eventData_["pid"] = std::to_string(instanceInfo.forwardCaller.pid);
            }
            AVCODEC_LOGD("Instance caller pid: %{public}s", eventData_.at("pid").c_str());
        }
        eventData_["decRate"] = std::to_string(static_cast<int>(reportDecFps));
        RSInterfaceAdapter::GetInstance().NotifyFrameRateEvent(eventData_);
    }
}

void DecodingBehaviorAnalyzer::HandleBehaviorTransition(DecodingBehaviorType currentType, size_t oldSpeedIdx,
    DecodingBehaviorType newType, size_t newSpeedIdx)
{
    if (IsDecChanged(currentType, oldSpeedIdx, newType, newSpeedIdx)) {
        std::chrono::steady_clock::time_point endTime = lastCheckTime_ != std::chrono::steady_clock::time_point{} ?
            lastCheckTime_ : std::chrono::steady_clock::now();
        AVCODEC_LOGD("type %{public}d, speed %{public}.2fx, newType %{public}d, new speed %{public}.2fx,",
            currentType, standardSpeeds[oldSpeedIdx], newType, standardSpeeds[newSpeedIdx]);
        SaveDecodingBehaviorAsSnapshot(currentType, endTime, oldSpeedIdx);
    }
}

void DecodingBehaviorAnalyzer::SetDecInfoCallBack(DecInfoCallBack cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    decInfoCallBack_ = std::move(cb);
}

} // namespace MediaAVCodec
} // namespace OHOS