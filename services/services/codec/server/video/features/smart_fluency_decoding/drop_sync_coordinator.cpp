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
#include "avcodec_log_ex.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SfdSync"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

bool DropSyncCoordinator::RecordPreDroppedFrame(int64_t pts)
{
    size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
    size_t nextWrite = currentWrite + 1;
    CHECK_AND_RETURN_RET_LOG((nextWrite - readIndex_.load(std::memory_order_acquire)) <= queueCapacity,
        false, "RecordPreDroppedFrame queue overflow at PTS %{public}" PRId64, pts);
    size_t index = currentWrite & queueMask;
    ptsRingBuffer_[index] = pts;
    isConsumed_[index].store(false, std::memory_order_release);
    writeIndex_.store(nextWrite, std::memory_order_release);
    UpdateFeedback(false, true);
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

void DropSyncCoordinator::UpdateFeedback(bool isRetained, bool isPreDropped)
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    if (feedbackWindowCount_ == feedbackWindowSize) {
        auto& old = feedbackWindow_[feedbackWindowPos_];
        if (old.isRetained) {
            windowRetainedCount_--;
        }
        if (old.isPreDropped) {
            windowPreDroppedCount_--;
        }
    } else {
        feedbackWindowCount_++;
    }
    auto& slot = feedbackWindow_[feedbackWindowPos_];
    slot.isRetained = isRetained;
    slot.isPreDropped = isPreDropped;
    if (isRetained) {
        windowRetainedCount_++;
    }
    if (isPreDropped) {
        windowPreDroppedCount_++;
    }
    feedbackWindowPos_ = (feedbackWindowPos_ + 1) & feedbackWindowMask;
    AVCODEC_LOGD("isRetained=%{public}d, isPreDropped=%{public}d, window=%{public}zu/%{public}zu"
        ", retained=%{public}u, preDropped=%{public}u",
        isRetained, isPreDropped, feedbackWindowCount_, feedbackWindowSize,
        windowRetainedCount_, windowPreDroppedCount_);
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
    feedbackWindowCount_ = 0;
    feedbackWindowPos_ = 0;
    windowRetainedCount_ = 0;
    windowPreDroppedCount_ = 0;
    AVCODEC_LOGI("Reset DropSyncCoordinator");
}

double DropSyncCoordinator::GetActualRetentionRatio() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    if (feedbackWindowCount_ == 0) {
        return 0.0;
    }
    return static_cast<double>(windowRetainedCount_) / feedbackWindowCount_;
}

size_t DropSyncCoordinator::GetWriteIndex() const
{
    return writeIndex_.load(std::memory_order_relaxed);
}

size_t DropSyncCoordinator::GetReadIndex() const
{
    return readIndex_.load(std::memory_order_relaxed);
}

double DropSyncCoordinator::GetPreRetainedRatio() const
{
    std::lock_guard<std::mutex> lock(feedbackMutex_);
    if (feedbackWindowCount_ == 0) {
        return 0.0;
    }
    return static_cast<double>(feedbackWindowCount_ - windowPreDroppedCount_) / feedbackWindowCount_;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS