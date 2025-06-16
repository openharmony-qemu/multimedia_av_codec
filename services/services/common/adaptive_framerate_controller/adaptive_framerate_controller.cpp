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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "EventManager"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
bool AdaptiveFramerateController::Initialize(std::function<void(double)> &&resetFramerateHandler)
{
    ResetFramerateHandler_ = std::move(resetFramerateHandler);
    return true;
}

void AdaptiveFramerateController::OnFrameConsumed()
{
    if (frameCount_ == 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!loopThread_) {
            isRunning_ = true;
            loopThread_ = std::make_unique<std::thread>(&AdaptiveFramerateController::Loop, this);
        }
        lastAdjustmentTime_ = std::chrono::steady_clock::now();
    }
    frameCount_++;
}

void AdaptiveFramerateController::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isRunning_ = false;
    if (loopThread_ && loopThread_->joinable()) {
        condition_.notify_all();
        lock.unlock();
        loopThread_->join();
        loopThread_.reset();
    }
    frameCount_.store(0);
}

bool AdaptiveFramerateController::ResetFramerate()
{
    auto frameCount = frameCount_.exchange(0);
    auto now = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(now - lastAdjustmentTime_).count();
    lastAdjustmentTime_ = now;
    if (elapsedTime <= 0) {
        AVCODEC_LOGW("Elapsed time is zero, skipping framerate reset.");
        return false;
    }
    auto framerate = static_cast<double>(frameCount) / elapsedTime;
    if (!(lastFramerate_ <= 0 || std::abs(framerate - lastFramerate_) / lastFramerate_ > 0.05)) {
        return true;
    }
    ResetFramerateHandler_(framerate);
    AVCODEC_LOGI("Reset framerate: %{public}.2f fps", framerate);
    lastFramerate_ = framerate;
    return true;
}

void AdaptiveFramerateController::Loop()
{
    using namespace std::chrono_literals;
    constexpr auto CheckInterval = 2s;
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, CheckInterval, [this]() { return !isRunning_; });
        if (!isRunning_) {
            break;
        }        
        ResetFramerate();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS