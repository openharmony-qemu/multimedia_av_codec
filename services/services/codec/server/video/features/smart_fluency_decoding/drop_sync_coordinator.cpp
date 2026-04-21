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
constexpr double BASE_ALPHA = 0.15;
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

bool DropSyncCoordinator::RecordPreDroppedFrame(int64_t pts, double speed)
{
    size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
    size_t nextWrite = currentWrite + 1;
    CHECK_AND_RETURN_RET_LOG((nextWrite - readIndex_.load(std::memory_order_acquire)) <= queueCapacity,
        false, "RecordPreDroppedFrame queue overflow at PTS %{public}" PRId64, pts);
    size_t index = currentWrite & queueMask;
    ptsRingBuffer_[index] = pts;
    isConsumed_[index].store(false, std::memory_order_release);
    writeIndex_.store(nextWrite, std::memory_order_release);
    UpdateFeedback(false, speed);
    AVCODEC_LOGD("Recorded pre-dropped PTS %{public}" PRId64 ", writeIndex: %{public}zu", pts, nextWrite);
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
        if (!isConsumed_[index].load(std::memory_order_acquire)) {
            if (ptsRingBuffer_[index] < currentPts) {
                count++;
                isConsumed_[index].store(true, std::memory_order_release);
            }
        }
        scanIndex++;
    }
    while (currentRead < currentWrite && isConsumed_[currentRead & queueMask].load(std::memory_order_acquire)) {
        currentRead++;
    }
    if (count > 0) {
        readIndex_.store(currentRead, std::memory_order_release);
        AVCODEC_LOGD("Consumed %{public}u pre-dropped frames before PTS %{public}" PRId64, count, currentPts);
    }
    return count;
}

void DropSyncCoordinator::UpdateFeedback(bool isRetained, double speed)
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    double alpha = std::min(1.0, BASE_ALPHA * speed);
    double currentVal = isRetained ? 1.0 : 0.0;
    emaRatio_ = alpha * currentVal + (1.0 - alpha) * emaRatio_;
    AVCODEC_LOGD("isRetained=%{public}d, emaRatio=%{public}.4f",
        isRetained, emaRatio_);
}

void DropSyncCoordinator::Reset()
{
    {
        size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
        size_t currentRead = readIndex_.load(std::memory_order_relaxed);
        while (currentRead < currentWrite) {
            isConsumed_[currentRead & queueMask].store(true, std::memory_order_relaxed);
            currentRead++;
        }
        readIndex_.store(currentWrite, std::memory_order_relaxed);
    }
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    emaRatio_ = 1.0;
    AVCODEC_LOGI("Reset DropSyncCoordinator");
}

double DropSyncCoordinator::GetEmaRatio() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    return emaRatio_;
}

size_t DropSyncCoordinator::GetWriteIndex() const
{
    return writeIndex_.load(std::memory_order_relaxed);
}

size_t DropSyncCoordinator::GetReadIndex() const
{
    return readIndex_.load(std::memory_order_relaxed);
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS