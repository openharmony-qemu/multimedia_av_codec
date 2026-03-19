/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef SAMPLE_QUEUE_CONTROLLER_H
#define SAMPLE_QUEUE_CONTROLLER_H

#include <atomic>
#include <memory>
#include <mutex>
#include "common/media_source.h"
#include "demuxer/sample_queue.h"
#include "osal/task/task.h"
#include "osal/utils/steady_clock.h"


namespace OHOS {
namespace Media {
struct SpeedCountInfo {
    static constexpr double TIME_TO_US = 1000000.0;
    std::atomic<uint64_t> totalFrameCount = 0;
    std::atomic<uint64_t> totalEffectiveRunTimeUs = 0;
    std::atomic<uint64_t> lastEventTimeUs = 0;
    uint64_t GetCurrentTimeUs();
    void IncrementFrameCount();
    void OnEventTimeRecord();
    double GetSpeed() const;
    uint64_t GetTotalFrameCount() const;
    uint64_t GetTotalEffectiveRunTimeUs() const;
};

class SampleQueueController {
public:
    explicit SampleQueueController();
    ~SampleQueueController() = default;
    uint64_t GetQueueSize(int32_t trackId);
    void SetQueueSize(int32_t trackId, uint64_t size);
    void AddQueueSize(int32_t trackId, uint64_t size);
    bool ShouldStartConsume(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
        const std::unique_ptr<Task> &task, bool inPreroll);
    bool ShouldStopConsume(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
        const std::unique_ptr<Task> &task);
    bool ShouldStartProduce(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
        const std::unique_ptr<Task> &task);
    bool ShouldStopProduce(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
        const std::unique_ptr<Task> &task);
    void ProduceIncrementFrameCount(int32_t trackId);
    void ProduceOnEventTimeRecord(int32_t trackId);
    void ConsumeSpeed(int32_t trackId);
    void SetSpeed(float speed);
    float GetSpeed();
    void SetBufferingDuration(std::shared_ptr<Plugins::PlayStrategy> strategy);
    uint64_t GetBufferingDuration();
    uint64_t GetPlayBufferingDuration();
    void DisableFirstBufferingDuration();

    static constexpr uint64_t QUEUE_SIZE_MIN = 30;
    static constexpr uint64_t START_CONSUME_WATER_LOOP = 5 * 1000 * 1000;
    static constexpr uint64_t STOP_CONSUME_WATER_LOOP = 0;
    static constexpr uint64_t START_PRODUCE_WATER_LOOP = 5 * 1000 * 1000;
    static constexpr uint64_t STOP_PRODUCE_WATER_LOOP = 10 * 1000 * 1000;
    static constexpr uint64_t FIRST_START_CONSUME_WATER_LOOP = 2 * 1000 * 1000;
    static constexpr uint32_t ADD_ONCE_MAX_SIZE = 100;

private:
    std::map<int32_t, std::shared_ptr<SpeedCountInfo>> produceSpeedCountInfo_;
    std::map<int32_t, std::shared_ptr<SpeedCountInfo>> consumeSpeedCountInfo_;
    std::map<int32_t, uint64_t> queueSizeMap_;
    std::map<int32_t, bool> isFirstArrived_;
    std::atomic<bool> isSetFirstBufferingDuration_ {};
    std::atomic<uint64_t> bufferingDuration_ {};
    std::atomic<uint64_t> firstBufferingDuration_ {};
    std::atomic<float> speed_ {1.0f};
    std::map<int32_t, int64_t> stopConsumeStartTime_;
};
} // namespace Media
} // namespace OHOS
#endif // SAMPLE_QUEUE_CONTROLLER_H