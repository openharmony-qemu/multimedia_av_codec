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

#ifndef MEDIA_AVCODEC_ADAPTIVE_FRAMERATE_CONTROLLER_H
#define MEDIA_AVCODEC_ADAPTIVE_FRAMERATE_CONTROLLER_H

#include <chrono>
#include <functional>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include "avcodec_dfx_component.h"

namespace OHOS {
namespace MediaAVCodec {
class FramerateCalculator : public std::enable_shared_from_this<FramerateCalculator>,
                            public AVCodecDfxComponent {
public:
    FramerateCalculator(int32_t instanceId, std::function<void(double)> &&resetFramerateHandler);
    void OnFrameConsumed();
    void OnStopped();
    bool CheckAndResetFramerate();

private:
    enum class Status {
        INITIALIZED,
        RUNNING,
        STOPPED,
    };
    void Register2AFC();
    void UnregisterFromAFC();

    int32_t instanceId_;
    std::atomic<Status> status_ = Status::INITIALIZED;
    std::function<void(double)> resetFramerateHandler_;
    std::atomic<uint32_t> frameCount_{0};
    double lastFramerate_{1.0};
    uint8_t decreseCheckTimes_{0};
    static constexpr uint8_t MAX_DECREASE_CHECK_TIMES = 3; // Maximum times
    std::chrono::steady_clock::time_point lastAdjustmentTime_{};
};

class AdaptiveFramerateController {
public:
    static AdaptiveFramerateController &GetInstance();
    void Add(int32_t intanceId, std::shared_ptr<FramerateCalculator> calculator);
    void Remove(int32_t instanceId);

private:
    void Loop();

    std::unordered_map<int32_t, std::weak_ptr<FramerateCalculator>> calculators_;
    std::unique_ptr<std::thread> looper_;
    std::atomic<bool> isRunning_ = false;
    std::mutex mutex_;
    std::condition_variable condition_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_ADAPTIVE_FRAMERATE_CONTROLLER_H