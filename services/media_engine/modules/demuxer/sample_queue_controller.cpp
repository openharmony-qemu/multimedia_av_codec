/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "SampleQueueController"

#include "common/log.h"
#include "sample_queue_controller.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_PLAYER, "SampleQueueController" };
constexpr uint32_t S_TO_US = 1000 * 1000;
constexpr double MIN_FIRST_DURATION = 0;
constexpr double MAX_FIRST_DURATION = 20;
constexpr uint64_t MIN_DURATION = 1;
constexpr uint64_t MAX_DURATION = 20;
}

namespace OHOS {
namespace Media {
SampleQueueController::SampleQueueController()
{
    MEDIA_LOG_D("SampleQueueController constructor");
}

uint64_t SampleQueueController::GetQueueSize(int32_t trackId)
{
    if (queueSizeMap_[trackId] < QUEUE_SIZE_MIN) {
        queueSizeMap_[trackId] = QUEUE_SIZE_MIN;
    }
    return queueSizeMap_[trackId];
}

void SampleQueueController::SetQueueSize(int32_t trackId, uint64_t size)
{
    queueSizeMap_[trackId] = size;
}

void SampleQueueController::AddQueueSize(int32_t trackId, uint64_t size)
{
    if (queueSizeMap_[trackId] < QUEUE_SIZE_MIN) {
        queueSizeMap_[trackId] = QUEUE_SIZE_MIN;
    }
    queueSizeMap_[trackId] += size;
}

bool SampleQueueController::ShouldStartConsume(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
    const std::unique_ptr<Task> &task, bool inPreroll)
{
    if (sampleQueue == nullptr || task == nullptr) {
        return false;
    }
    uint64_t cacheDuration = sampleQueue->NewGetCacheDuration();
    if (cacheDuration < GetPlayBufferingDuration() &&
        sampleQueue->GetFilledBufferSize() < SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE - 1 &&
        (isFirstArrived_[trackId] ||cacheDuration < static_cast<uint64_t>(FIRST_START_CONSUME_WATER_LOOP)) &&
        !inPreroll) {
        return false;
    }
    DisableFirstBufferingDuration();

    if (!task->IsTaskRunning()) {
        MEDIA_LOG_I("StartConsume, cacheDuration: %{public}llu, trackId: %{public}d", cacheDuration, trackId);
        task->Start();
        isFirstArrived_[trackId] = true;
    }
    return true;
}

bool SampleQueueController::ShouldStopConsume(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
    const std::unique_ptr<Task> &task)
{
    if (sampleQueue == nullptr || task == nullptr) {
        return false;
    }
    uint64_t cacheDuration = sampleQueue->NewGetCacheDuration();
    if (cacheDuration > STOP_CONSUME_WATER_LOOP) {
        return false;
    }
    if (task->IsTaskRunning()) {
        MEDIA_LOG_I("StopConsume, cacheDuration: %{public}llu, trackId: %{public}d", cacheDuration, trackId);
        task->Pause();
    }
    return true;
}

bool SampleQueueController::ShouldStartProduce(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
    const std::unique_ptr<Task> &task)
{
    if (sampleQueue == nullptr || task == nullptr) {
        return false;
    }
    uint64_t cacheDuration = sampleQueue->NewGetCacheDuration();
    if (cacheDuration > GetPlayBufferingDuration()) {
        return false;
    }
    if (!task->IsTaskRunning()) {
        MEDIA_LOG_I("StartProduce, cacheDuration: %{public}llu, trackId: %{public}d", cacheDuration, trackId);
        task->Start();
    }
    return true;
}

bool SampleQueueController::ShouldStopProduce(int32_t trackId, std::shared_ptr<SampleQueue> sampleQueue,
    const std::unique_ptr<Task> &task)
{
    if (sampleQueue == nullptr || task == nullptr) {
        return false;
    }
    uint64_t cacheDuration = sampleQueue->NewGetCacheDuration();
    if (cacheDuration < GetBufferingDuration() &&
        sampleQueue->GetFilledBufferSize() < SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE - 1) {
        return false;
    }

    if (task->IsTaskRunning()) {
        MEDIA_LOG_I("StopProduce, cacheDuration: %{public}llu, trackId: %{public}d", cacheDuration, trackId);
        task->Pause();
    }
    return true;
}

void SampleQueueController::SetSpeed(float speed)
{
    speed_.store(speed);
}

float SampleQueueController::GetSpeed()
{
    return speed_.load();
}

void SampleQueueController::ProduceIncrementFrameCount(int32_t trackId)
{
    if (produceSpeedCountInfo_[trackId] == nullptr) {
        produceSpeedCountInfo_[trackId] = std::make_shared<SpeedCountInfo>();
    }
    auto countInfo = produceSpeedCountInfo_[trackId];
    countInfo->IncrementFrameCount();
}

void SampleQueueController::ProduceOnEventTimeRecord(int32_t trackId)
{
    if (produceSpeedCountInfo_[trackId] == nullptr) {
        produceSpeedCountInfo_[trackId] = std::make_shared<SpeedCountInfo>();
    }
    auto countInfo = produceSpeedCountInfo_[trackId];
    countInfo->OnEventTimeRecord();
}

void SampleQueueController::ConsumeSpeed(int32_t trackId)
{
    if (consumeSpeedCountInfo_[trackId] == nullptr) {
        consumeSpeedCountInfo_[trackId] = std::make_shared<SpeedCountInfo>();
    }
    auto countInfo = consumeSpeedCountInfo_[trackId];
    countInfo->IncrementFrameCount();
    countInfo->OnEventTimeRecord();
}

void SampleQueueController::SetBufferingDuration(std::shared_ptr<Plugins::PlayStrategy> strategy)
{
    if (strategy == nullptr) {
        MEDIA_LOG_W("SetBufferingDuration strategy nullptr!");
        return;
    }
    if (strategy->duration != 0 && strategy->bufferDurationForPlaying != 0 &&
        strategy->duration < strategy->bufferDurationForPlaying) {
        MEDIA_LOG_E("Buffer duration for playing must be less than total playback duration.");
        return;
    }
    if (strategy->duration != 0) {
        bufferingDuration_.store(std::max(MIN_DURATION,
            std::min(MAX_DURATION, static_cast<uint64_t>(strategy->duration))) * S_TO_US);
    }
    if (strategy->bufferDurationForPlaying != 0) {
        isSetFirstBufferingDuration_.store(true);
        firstBufferingDuration_.store(static_cast<uint64_t>(std::max(MIN_FIRST_DURATION,
            std::min(MAX_FIRST_DURATION, strategy->bufferDurationForPlaying))) * S_TO_US);
    }
    MEDIA_LOG_I("set duration, enable first: " PUBLIC_LOG_D32 ", first: " PUBLIC_LOG_U64 ", playing duration: "
        PUBLIC_LOG_U64, isSetFirstBufferingDuration_.load(), firstBufferingDuration_.load(),
        bufferingDuration_.load());
}

uint64_t SampleQueueController::GetBufferingDuration()
{
    return bufferingDuration_.load() > 0 ? bufferingDuration_.load() : STOP_PRODUCE_WATER_LOOP;
}

uint64_t SampleQueueController::GetPlayBufferingDuration()
{
    return isSetFirstBufferingDuration_.load() ? firstBufferingDuration_.load() : START_CONSUME_WATER_LOOP;
}

void SampleQueueController::DisableFirstBufferingDuration()
{
    if (!isSetFirstBufferingDuration_.load()) {
        return;
    }
    MEDIA_LOG_I("disable first buffering duration");
    isSetFirstBufferingDuration_.store(false);
}

uint64_t SpeedCountInfo::GetCurrentTimeUs()
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return duration.count();
}

void SpeedCountInfo::IncrementFrameCount()
{
    totalFrameCount.fetch_add(1, std::memory_order_relaxed);
}

void SpeedCountInfo::OnEventTimeRecord()
{
    auto now = GetCurrentTimeUs();
    auto lastTime = lastEventTimeUs.load();
    if (lastTime > 0) {
        auto delta = now - lastTime;
        totalEffectiveRunTimeUs.fetch_add(delta, std::memory_order_relaxed);
    }
    lastEventTimeUs.store(now, std::memory_order_relaxed);
}

double SpeedCountInfo::GetSpeed() const
{
    auto effectiveTimeUs = totalEffectiveRunTimeUs.load();
    if (effectiveTimeUs == 0) {
        return 0.0;
    }
    auto frameCount = totalFrameCount.load();
    return static_cast<double>(frameCount) * TIME_TO_US / effectiveTimeUs;
}

uint64_t SpeedCountInfo::GetTotalFrameCount() const
{
    return totalFrameCount.load();
}

uint64_t SpeedCountInfo::GetTotalEffectiveRunTimeUs() const
{
    return totalEffectiveRunTimeUs.load();
}
} // namespace Media
} // namespace OHOS
