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

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
bool DropSyncCoordinator::RecordPreDroppedFrame(int64_t droppedPts)
{
    size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
    size_t nextWrite = currentWrite + 1;
    if ((nextWrite - readIndex_.load(std::memory_order_acquire)) > queueCapacity) {
        return false;
    }
    ptsRingBuffer_[currentWrite & queueMask] = droppedPts;
    writeIndex_.store(nextWrite, std::memory_order_release);
    return true;
}

int32_t DropSyncCoordinator::GetAndConsumePreDroppedCount(int64_t currentPts)
{
    int32_t count = 0;
    size_t currentRead = readIndex_.load(std::memory_order_relaxed);
    size_t currentWrite = writeIndex_.load(std::memory_order_acquire);
    while (currentRead < currentWrite) {
        int64_t peekPts = ptsRingBuffer_[currentRead & queueMask];
        if (peekPts < currentPts) {
            count++;
            currentRead++;
        } else {
            break;
        }
    }
    if (count > 0) {
        readIndex_.store(currentRead, std::memory_order_release);
    }
    return count;
}

void DropSyncCoordinator::Reset()
{
    writeIndex_.store(0, std::memory_order_relaxed);
    readIndex_.store(0, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    currentEmaDropRatio_ = 0.0;
}

double DropSyncCoordinator::GetCurrentEma() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    return currentEmaDropRatio_;
}

double DropSyncCoordinator::GetTargetRetentionRatio() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    return targetRetentionRatio_;
}

void DropSyncCoordinator::UpdateFeedback(int32_t preDroppedCount, bool currentDropped,
                                         double targetRetentionRatio, double currentSpeed)
{
    double baseAlpha = 0.15;
    double alpha = std::min(1.0, baseAlpha * currentSpeed);
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    for (int32_t i = 0; i < preDroppedCount; ++i) {
        currentEmaDropRatio_ = alpha * 1.0 + (1.0 - alpha) * currentEmaDropRatio_;
    }
    double currentVal = currentDropped ? 1.0 : 0.0;
    currentEmaDropRatio_ = alpha * currentVal + (1.0 - alpha) * currentEmaDropRatio_;
    targetRetentionRatio_ = targetRetentionRatio;
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS