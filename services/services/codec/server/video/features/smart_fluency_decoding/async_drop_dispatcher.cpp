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

#include "async_drop_dispatcher.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

AsyncDropDispatcher::AsyncDropDispatcher()
{
    isWorking_.store(true);
    workerThread_ = std::thread(&AsyncDropDispatcher::WorkerLoop, this);
}

AsyncDropDispatcher::~AsyncDropDispatcher()
{
    StopWorker();
}

void AsyncDropDispatcher::StopWorker()
{
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        if (!isWorking_.load()) {
            return;
        }
        isWorking_.store(false);
    }
    taskCv_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void AsyncDropDispatcher::SubmitTask(std::function<void()> task)
{
    if (!task) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        if (!isWorking_.load()) {
            return;
        }
        taskQueue_.push(std::move(task));
    }
    taskCv_.notify_one();
}

void AsyncDropDispatcher::WorkerLoop()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(taskMutex_);
            taskCv_.wait(lock, [this]() {
                return !isWorking_.load() || !taskQueue_.empty();
            });
            if (!isWorking_.load() && taskQueue_.empty()) {
                break;
            }
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }
        if (task) {
            task();
        }
    }
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
