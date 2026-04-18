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

#ifndef DROP_SYNC_COORDINATOR_H
#define DROP_SYNC_COORDINATOR_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
class DropSyncCoordinator {
public:
    DropSyncCoordinator() = default;
    ~DropSyncCoordinator() = default;

    bool RecordPreDroppedFrame(int64_t pts, double speed);
    uint32_t GetAndConsumePreDroppedCount(int64_t currentPts);
    void Reset();
    
    double GetCurrentEma() const;
    double GetTargetRetentionRatio() const;
    void UpdateFeedback(bool isDropped, double targetRetentionRatio, double speed);

    size_t GetWriteIndex() const { return writeIndex_.load(std::memory_order_relaxed); }
    size_t GetReadIndex() const { return readIndex_.load(std::memory_order_relaxed); }

private:
    // ????:??????? feedbackMutex_ ?
    void UpdateEmaDropRatio(bool isDropped, double speed);

    static constexpr size_t queueCapacity = 128;
    static constexpr size_t queueMask = queueCapacity - 1;
    static constexpr size_t cacheLineSize = 64;
    
    int64_t ptsRingBuffer_[queueCapacity]{0};
    bool isConsumed_[queueCapacity]{false}; // ????????????????????
    
    alignas(cacheLineSize) std::atomic<size_t> writeIndex_{0};
    alignas(cacheLineSize) std::atomic<size_t> readIndex_{0};
    
    double currentEmaDropRatio_{0.0};
    double targetRetentionRatio_{0.0};
    mutable std::mutex feedbackMutex_;
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // DROP_SYNC_COORDINATOR_H