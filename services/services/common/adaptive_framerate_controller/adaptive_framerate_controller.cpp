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

#include "adaptive_framerate_controller.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AdaptiveFramerateController"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
FramerateCalculator::FramerateCalculator(int32_t instanceId, std::function<void(double)> &&resetFramerateHandler)
    : instanceId_(instanceId), status_(Status::INITIALIZED), resetFramerateHandler_(std::move(resetFramerateHandler)) {}

void FramerateCalculator::OnFrameConsumed()
{
    if (status_ != Status::UNINITIALIZED) {
        return;
    } else if (status_ == Status::INITIALIZED || status_ == Status::STOPPED) {
        lastAdjustmentTime_ = std::chrono::steady_clock::now();
        Register2AFC();
        status_ = Status::RUNNING;
    }
    frameCount_++;
}

void FramerateCalculator::OnStopped()
{
    status_ = Status::STOPPED;
    UnregisterFromAFC();
}

bool FramerateCalculator::CheckAndResetFramerate()
{
    auto frameCount = frameCount_.exchange(0);
    auto now = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - lastAdjustmentTime_).count();
    lastAdjustmentTime_ = now;
    CHECK_AND_RETURN_RET_LOGW(elapsedTime > 0, false, "Elapsed time is invalid, cannot calculate framerate");

    auto framerate = static_cast<double>(frameCount) / elapsedTime * 1000;  // 1000: milliseconds to seconds
    if (!(lastFramerate_ <= 0 || std::abs(framerate - lastFramerate_) / lastFramerate_ > 0.1)) {  // 0.1: 10% threshold
        return false;
    }
    resetFramerateHandler_(framerate);

    char direction = (framerate > lastFramerate_) ? '+' : '-';
    AVCODEC_LOGD("Reset framerate: %{public}.2ffps %{public}c, frame count: %{public}u, elapsed: %{public}lldms",
        framerate, direction, frameCount, elapsedTime);

    lastFramerate_ = framerate;
    return true;
}

void FramerateCalculator::Register2AFC()
{
    auto &afc = AdaptiveFramerateController::GetInstance();
    afc.Add(instanceId_, shared_from_this());
}

void FramerateCalculator::UnregisterFromAFC()
{
    auto &afc = AdaptiveFramerateController::GetInstance();
    afc.Remove(instanceId_);
}

AdaptiveFramerateController &AdaptiveFramerateController::GetInstance()
{
    static AdaptiveFramerateController instance;
    return instance;
}

void AdaptiveFramerateController::Add(int32_t intanceId, std::shared_ptr<FramerateCalculator> calculator)
{
    std::lock_guard<std::mutex> lock(mutex_);
    calculators_[intanceId] = calculator;
    if (!isRunning_) {
        isRunning_ = true;
        if (!looper_) {
            looper_ = std::make_unique<std::thread>(&AdaptiveFramerateController::Loop, this);
        }
    }
}

void AdaptiveFramerateController::Remove(int32_t instanceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    calculators_.erase(instanceId);
    if (calculators_.empty()) {
        isRunning_ = false;
        condition_.notify_all();
        if (looper_ && looper_->joinable()) {
            looper_->join();
            looper_.reset();
        }
    }
}

void AdaptiveFramerateController::Loop()
{
    using namespace std::chrono_literals;
    constexpr auto CheckInterval = 1s;
    AVCODEC_LOGD("started");
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, CheckInterval, [this]() { return !isRunning_; });
        if (!isRunning_) {
            break;
        }
        for (auto it = calculators_.begin(); it != calculators_.end();) {
            auto calculator = it->second.lock();
            if (!calculator) {
                it = calculators_.erase(it);
                continue;
            }
            calculator->CheckAndResetFramerate();
            ++it;
        }
        if (calculators_.empty()) {
            isRunning_ = false;
            break;
        }
    }
    AVCODEC_LOGD("stopped");
}
} // namespace MediaAVCodec
} // namespace OHOS