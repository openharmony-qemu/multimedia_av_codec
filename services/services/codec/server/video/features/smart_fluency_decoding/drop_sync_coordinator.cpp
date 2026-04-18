/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "drop_sync_coordinator.h"
#include <algorithm>
#include "avcodec_log_ex.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SfdSync"};
constexpr double BASE_ALPHA = 0.15; // ??????
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

bool DropSyncCoordinator::RecordPreDroppedFrame(int64_t pts, double speed)
{
    size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
    size_t nextWrite = currentWrite + 1;
    if ((nextWrite - readIndex_.load(std::memory_order_acquire)) > queueCapacity) {
        AVCODEC_LOGW("SfdSync: RecordPreDroppedFrame queue overflow at PTS %{public}" PRId64, pts);
        return false;
    }
    size_t index = currentWrite & queueMask;
    ptsRingBuffer_[index] = pts;
    isConsumed_[index] = false; // ????????????
    writeIndex_.store(nextWrite, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(feedbackMutex_);
        // 改为调用新的保留率统计：丢帧代表本帧 isRetained = false
        UpdateEmaRetentionRatio(false, speed);
    }
    AVCODEC_LOGD("SfdSync: Recorded pre-dropped PTS %{public}" PRId64 ", writeIndex: %{public}zu", pts, nextWrite);
    return true;
}

uint32_t DropSyncCoordinator::GetAndConsumePreDroppedCount(int64_t currentPts)
{
    uint32_t count = 0;
    size_t currentRead = readIndex_.load(std::memory_order_relaxed);
    size_t currentWrite = writeIndex_.load(std::memory_order_acquire);
    size_t scanIndex = currentRead;

    while (scanIndex < currentWrite) {
        size_t index = scanIndex & queueMask;
        if (!isConsumed_[index]) {
            if (ptsRingBuffer_[index] < currentPts) {
                count++;
                isConsumed_[index] = true;
            }
        }
        scanIndex++;
    }
    while (currentRead < currentWrite && isConsumed_[currentRead & queueMask]) {
        currentRead++;
    }
    if (count > 0) {
        readIndex_.store(currentRead, std::memory_order_release);
        AVCODEC_LOGD("SfdSync: Consumed %{public}u pre-dropped frames before PTS %{public}" PRId64, count, currentPts);
    }
    return count;
}

void DropSyncCoordinator::UpdateFeedback(bool isRetained, double targetRetentionRatio, double speed)
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    UpdateEmaRetentionRatio(isRetained, speed);
    targetRetentionRatio_ = targetRetentionRatio;
    AVCODEC_LOGD("SfdSync: UpdateFeedback: isRetained=%{public}d, targetRat=%{public}.2f, currEmaRet=%{public}.4f",
                 isRetained, targetRetentionRatio_, currentEmaRetentionRatio_);
}

void DropSyncCoordinator::UpdateEmaRetentionRatio(bool isRetained, double speed)
{
    double alpha = std::min(1.0, BASE_ALPHA * speed);
    // 保留率为 1.0 (保留) 或 0.0 (丢弃)
    double currentVal = isRetained ? 1.0 : 0.0;
    currentEmaRetentionRatio_ = alpha * currentVal + (1.0 - alpha) * currentEmaRetentionRatio_;
}

void DropSyncCoordinator::Reset()
{
    writeIndex_.store(0, std::memory_order_relaxed);
    readIndex_.store(0, std::memory_order_relaxed);
    for (size_t i = 0; i < queueCapacity; ++i) {
        isConsumed_[i] = false;
    }
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    // 重置为 100% 保留
    currentEmaRetentionRatio_ = 1.0;
    targetRetentionRatio_ = 1.0;
    AVCODEC_LOGI("SfdSync: Reset DropSyncCoordinator");
}

double DropSyncCoordinator::GetCurrentEmaRetentionRatio() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    return currentEmaRetentionRatio_;
}

double DropSyncCoordinator::GetTargetRetentionRatio() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    return targetRetentionRatio_;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS