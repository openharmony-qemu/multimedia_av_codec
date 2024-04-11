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

#include "task_thread.h"
#include "avcodec_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "TaskThread"};
    constexpr uint8_t LOGD_FREQUENCY = 100;
}
namespace OHOS {
namespace MediaAVCodec {
TaskThread::TaskThread(std::string_view name) : name_(name), runningState_(RunningState::STOPPED), loop_(nullptr)
{
    AVCODEC_LOGD("task %{public}s ctor called", name_.data());
}

TaskThread::TaskThread(std::string_view name, std::function<void()> handler) : TaskThread(name)
{
    handler_ = std::move(handler);
    loop_ = std::make_unique<std::thread>(&TaskThread::Run, this);
}

TaskThread::~TaskThread()
{
    AVCODEC_LOGD("task %{public}s dtor called", name_.data());
    runningState_ = RunningState::STOPPED;
    syncCond_.notify_all();

    if (loop_) {
        if (loop_->joinable()) {
            loop_->join();
        }
        loop_ = nullptr;
    }
}

void TaskThread::Start()
{
    std::unique_lock lock(stateMutex_);
    if (runningState_.load() == RunningState::STOPPING) {
        syncCond_.wait(lock, [this] { return runningState_.load() == RunningState::STOPPED; });
    }
    if (runningState_.load() == RunningState::STOPPED) {
        if (loop_) {
            if (loop_->joinable()) {
                loop_->join();
            }
            loop_ = nullptr;
        }
    }

    runningState_ = RunningState::STARTED;

    if (!loop_) { // thread not exist
        loop_ = std::make_unique<std::thread>(&TaskThread::Run, this);
    }
    syncCond_.notify_all();
    AVCODEC_LOGD("task %{public}s start called", name_.data());
}

void TaskThread::Stop()
{
    AVCODEC_LOGD("task %{public}s stop entered, current state: %{public}d", name_.data(), runningState_.load());
    std::unique_lock lock(stateMutex_);
    if (runningState_.load() != RunningState::STOPPED) {
        runningState_ = RunningState::STOPPING;
        syncCond_.notify_all();
        syncCond_.wait(lock, [this] { return runningState_.load() == RunningState::STOPPED; });
        if (loop_) {
            if (loop_->joinable()) {
                loop_->join();
            }
            loop_ = nullptr;
        }
    }
    AVCODEC_LOGD("task %{public}s stop exited", name_.data());
}

void TaskThread::StopAsync()
{
    AVCODEC_LOGD("task %{public}s StopAsync called", name_.data());
    std::unique_lock lock(stateMutex_);
    if (runningState_.load() != RunningState::STOPPING || runningState_.load() != RunningState::STOPPED) {
        runningState_ = RunningState::STOPPING;
        syncCond_.notify_all();
    }
}

void TaskThread::Pause()
{
    AVCODEC_LOGD("task %{public}s Pause called", name_.data());
    std::unique_lock lock(stateMutex_);
    switch (runningState_.load()) {
        case RunningState::STARTED: {
            runningState_ = RunningState::PAUSING;
            syncCond_.wait(lock, [this] {
                return runningState_.load() == RunningState::PAUSED || runningState_.load() == RunningState::STOPPED;
            });
            break;
        }
        case RunningState::STOPPING: {
            syncCond_.wait(lock, [this] { return runningState_.load() == RunningState::STOPPED; });
            break;
        }
        case RunningState::PAUSING: {
            syncCond_.wait(lock, [this] { return runningState_.load() == RunningState::PAUSED; });
            break;
        }
        default:
            break;
    }
    AVCODEC_LOGD("task %{public}s Pause done.", name_.data());
}

void TaskThread::PauseAsync()
{
    AVCODEC_LOGD("task %{public}s PauseAsync called", name_.data());
    std::unique_lock lock(stateMutex_);
    if (runningState_.load() == RunningState::STARTED) {
        runningState_ = RunningState::PAUSING;
    }
}

void TaskThread::RegisterHandler(std::function<void()> handler)
{
    AVCODEC_LOGD("task %{public}s RegisterHandler called", name_.data());
    handler_ = std::move(handler);
}

void TaskThread::doTask()
{
    AVCODEC_LOGD("task %{public}s not override DoTask...", name_.data());
}

void TaskThread::Run()
{
    // The max length for a thread name is 16.
    auto ret = pthread_setname_np(pthread_self(), name_.data());
    if (ret != 0) {
        AVCODEC_LOGE("task %{public}s set name failed", name_.data());
    }
    for (;;) {
        AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "task %{public}s is running on state : %{public}d",
            name_.data(), runningState_.load());
        if (runningState_.load() == RunningState::STARTED) {
            handler_();
        }
        std::unique_lock lock(stateMutex_);
        if (runningState_.load() == RunningState::PAUSING || runningState_.load() == RunningState::PAUSED) {
            runningState_ = RunningState::PAUSED;
            syncCond_.notify_all();
            constexpr int timeoutMs = 500;
            syncCond_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                               [this] { return runningState_.load() != RunningState::PAUSED; });
        }
        if (runningState_.load() == RunningState::STOPPING || runningState_.load() == RunningState::STOPPED) {
            runningState_ = RunningState::STOPPED;
            syncCond_.notify_all();
            break;
        }
    }
}
} // namespace MediaAVCodec
} // namespace OHOS