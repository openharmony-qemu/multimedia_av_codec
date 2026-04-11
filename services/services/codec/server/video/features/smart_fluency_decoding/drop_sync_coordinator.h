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

/**
 * @brief 智能流场控制器的反馈与同步追踪中心
 * 负责前置丢帧（Pre-dropped）跨线程同步，以及 EMA 闭环反馈状态的维护
 * TODO: 当前用PTS排序，后续支持低时延模式时，支持切换为DTS排序
 */
class DropSyncCoordinator {
public:
    DropSyncCoordinator() = default;
    ~DropSyncCoordinator() = default;

    /**
     * @brief 记录在解码前被丢弃的帧的 PTS
     * @note 仅由生产者（Demuxer/Parser 线程）调用
     * @param dropped_pts 被丢弃帧的媒体时间戳
     * @return true 记录成功，false 队列已满
     */
    bool RecordPreDroppedFrame(int64_t droppedPts);

    /**
     * @brief 获取并消费在当前 PTS 之前被丢弃的帧数
     * @note 仅由消费者（Decoder/Render 线程）调用
     * @param current_pts 当前正在处理的解码输出帧的媒体时间戳
     * @return 在 current_pts 之前被前置丢弃的帧数
     */
    int32_t GetAndConsumePreDroppedCount(int64_t currentPts);

    /**
     * @brief 重置同步器状态（通常在 Seek 或 Flush 时调用）
     */
    void Reset();

    /**
     * @brief 获取当前 EMA 实际丢帧率
     */
    double GetCurrentEma() const;

    /**
     * @brief 获取当前后端推荐的目标保留率
     */
    double GetTargetRetentionRatio() const;

    /**
     * @brief 更新 EMA 反馈状态
     */
    void UpdateFeedback(int32_t preDroppedCount, bool currentDropped, double targetRetentionRatio, double currentSpeed);

private:
    // 环形队列容量，必须是 2 的幂次，方便使用位掩码
    static constexpr size_t QUEUE_CAPACITY = 128; 
    static constexpr size_t QUEUE_MASK = QUEUE_CAPACITY - 1;

    int64_t ptsRingBuffer_[QUEUE_CAPACITY]{0};
    
    // 缓存行隔离，防止多核伪共享 (False Sharing)
    alignas(64) std::atomic<size_t> writeIndex_{0};
    alignas(64) std::atomic<size_t> readIndex_{0};

    // 跨线程动态反馈面板
    double currentEmaDropRatio_{0.0};
    double targetRetentionRatio_{0.0};
    mutable std::mutex feedbackMutex_;
};

#endif // DROP_SYNC_COORDINATOR_H