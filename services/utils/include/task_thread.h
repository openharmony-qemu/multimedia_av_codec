/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AV_CODEC_TASK_THREAD_H
#define AV_CODEC_TASK_THREAD_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

namespace OHOS {
namespace MediaAVCodec {
class __attribute__((visibility("default"))) TaskThread {
public:
    explicit TaskThread(std::string_view name);

    TaskThread(std::string_view name, std::function<void()> handler);

    ~TaskThread();

    void Start();

    void Stop();

    void StopAsync();

    void Pause();

    void PauseAsync();

    void RegisterHandler(std::function<void()> handler);

private:
    void doTask();
    void Run();

private:
    enum class RunningState {
        STARTED,
        PAUSING,
        PAUSED,
        STOPPING,
        STOPPED,
    };
    const std::string_view name_;
    std::atomic<RunningState> runningState_;
    std::unique_ptr<std::thread> loop_;
    std::function<void()> handler_ = [this] { doTask(); };
    std::mutex stateMutex_;
    std::condition_variable syncCond_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif