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

namespace OHOS {
namespace MediaAVCodec {
class AdaptiveFramerateController {
public:
    bool Initialize(std::function<void(double)> &&resetFramerateHandler);
    void OnFrameConsumed();
    void Stop();

private:
    bool ResetFramerate();
    void Loop();

    std::function<void(double)> ResetFramerateHandler_;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> loopThread_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<uint32_t> frameCount_{0};
    double lastFramerate_{0.0};
    std::chrono::steady_clock::time_point lastAdjustmentTime_{};
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_ADAPTIVE_FRAMERATE_CONTROLLER_H