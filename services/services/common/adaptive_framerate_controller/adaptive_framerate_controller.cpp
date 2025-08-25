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
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AFC"};
auto afcEnable = OHOS::system::GetBoolParameter("OHOS.MediaAVCodec.AFC.Enable", true);
using namespace std::chrono_literals;
constexpr auto CHECK_INTERVAL = 1000ms;
constexpr auto CHECK_INTERVAL_FILTER = (CHECK_INTERVAL / 2).count(); // CHECK_INTERVAL / 2: filter out short intervals
constexpr double DEFAULT_FRAMERATE = 60;
constexpr uint8_t MAX_INCREASE_CHECK_TIMES = 1;
constexpr uint8_t MAX_DECREASE_CHECK_TIMES = 2;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
FramerateCalculator::FramerateCalculator(int32_t instanceId, std::function<void(double)> &&handler)
    : instanceId_(instanceId), resetFramerateHandler_(std::move(handler)) {}

void FramerateCalculator::OnFrameConsumed()
{
    if (!afcEnable) {
        return;
    }
    if (status_ == Status::INITIALIZED || status_ == Status::STOPPED) {
        lastAdjustmentTime_ = std::chrono::steady_clock::now();
        Register2AFC();
        status_ = Status::RUNNING;
    }
    frameCount_++;
}

void FramerateCalculator::OnStopped()
{
    status_ = Status::STOPPED;
    frameCount_ = 0;
    UnregisterFromAFC();
}

bool FramerateCalculator::CheckAndResetFramerate()
{
    auto frameCount = frameCount_.exchange(0);
    auto now = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAdjustmentTime_).count();
    lastAdjustmentTime_ = now;
    CHECK_AND_RETURN_RET_LOGD_WITH_TAG(elapsedTime > CHECK_INTERVAL_FILTER, false,
        "Elapsed time is invalid, cannot calculate framerate");

    auto actualFramerate = static_cast<double>(frameCount) / elapsedTime * 1000;  // 1000: milliseconds to seconds
    if (actualFramerate < DEFAULT_FRAMERATE && actualFramerate < configuredFramerate_) {
        actualFramerate = std::min(DEFAULT_FRAMERATE, configuredFramerate_);
    }
    auto fluctuationFramerate = std::abs(actualFramerate - lastFramerate_);
    if (!(fluctuationFramerate > 5 && (fluctuationFramerate / lastFramerate_ > 0.1))) { // 5/0.1: reset threshold
        if (decreseCheckTimes_ != MAX_DECREASE_CHECK_TIMES) {
            decreseCheckTimes_ = MAX_DECREASE_CHECK_TIMES;
        }
        if (increseCheckTimes_ != MAX_INCREASE_CHECK_TIMES) {
            increseCheckTimes_ = MAX_INCREASE_CHECK_TIMES;
        }
        return false;
    }
    auto resetFramerate = actualFramerate;
    if (actualFramerate > lastFramerate_) {
        decreseCheckTimes_ = MAX_DECREASE_CHECK_TIMES;
        if (actualFramerate <= configuredFramerate_) {
            resetFramerate = configuredFramerate_;
        } else if (increseCheckTimes_ > 0) {
            increseCheckTimes_--;
            return false;
        } else {
            resetFramerate *= 2.5; // 2.5: increase factor
        }
    } else if (decreseCheckTimes_ > 0) {
        decreseCheckTimes_--;
        increseCheckTimes_ = MAX_INCREASE_CHECK_TIMES;
        return false;
    }
    resetFramerateHandler_(resetFramerate);

    char direction = (resetFramerate > lastFramerate_) ? '+' : '-';
    AVCODEC_LOGD_WITH_TAG("Reset framerate: %{public}.2f -> %{public}.2ffps(%{public}.2f) %{public}c",
        lastFramerate_.load(), resetFramerate, actualFramerate, direction);
    lastFramerate_ = resetFramerate;
    return true;
}

void FramerateCalculator::SetConfiguredFramerate(double framerate)
{
    if (framerate >= 1.0) { // 1.0: minimum framerate
        configuredFramerate_ = framerate;
        lastFramerate_ = configuredFramerate_;
    }
}

bool FramerateCalculator::SetFramerate2ConfiguredFramerate()
{
    resetFramerateHandler_(configuredFramerate_);
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
    {
        std::lock_guard<std::mutex> calculatorsLock(calculatorsMutex_);
        calculators_[intanceId] = calculator;
        if (isRunning_) {
            return;
        }
    }

    std::lock_guard<std::mutex> looperLock(looperMutex_);
    {
        std::lock_guard<std::mutex> signalLock(signalMutex_);
        isRunning_ = true;
    }
    if (!looper_) {
        looper_ = std::make_unique<std::thread>(&AdaptiveFramerateController::Loop, this);
    }
}

void AdaptiveFramerateController::Remove(int32_t instanceId)
{
    {
        std::lock_guard<std::mutex> calculatorsLock(calculatorsMutex_);
        calculators_.erase(instanceId);
        if (!calculators_.empty()) {
            return;
        }
    }

    std::lock_guard<std::mutex> looperLock(looperMutex_);
    {
        std::lock_guard<std::mutex> signalLock(signalMutex_);
        isRunning_ = false;
        condition_.notify_all();
    }
    if (looper_ && looper_->joinable()) {
        looper_->join();
    }
    looper_.reset();
}

void AdaptiveFramerateController::Loop()
{
    pthread_setname_np(pthread_self(), "OS_AFC_Loop");
    while (true) {
        std::unique_lock<std::mutex> signalLock(signalMutex_);
        condition_.wait_for(signalLock, CHECK_INTERVAL, [this]() { return !isRunning_; });
        if (!isRunning_) {
            break;
        }
        std::lock_guard<std::mutex> calculatorsLock(calculatorsMutex_);
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
}
} // namespace MediaAVCodec
} // namespace OHOS