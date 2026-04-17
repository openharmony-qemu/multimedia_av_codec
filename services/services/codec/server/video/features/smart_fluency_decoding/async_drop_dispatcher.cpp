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
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AsyncDropDispatcher"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
AsyncDropDispatcher::AsyncDropDispatcher()
{
    AVCODEC_LOGD("AsyncDropDispatcher constructor entry");
    isWorking_.store(true);
    workerThread_ = std::thread(&AsyncDropDispatcher::WorkerLoop, this);
}

AsyncDropDispatcher::~AsyncDropDispatcher()
{
    AVCODEC_LOGD("AsyncDropDispatcher destructor entry");
    StopWorker();
}

void AsyncDropDispatcher::StopWorker()
{
    AVCODEC_LOGD("StopWorker called");
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
    AVCODEC_LOGD("StopWorker completed, thread joined");
}

void AsyncDropDispatcher::SubmitTask(std::function<void()> task)
{
    if (!task) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        if (!isWorking_.load()) {
            AVCODEC_LOGW("SubmitTask failed, dispatcher not working");
            return;
        }
        taskQueue_.push(std::move(task));
        AVCODEC_LOGD("Task submitted, current queue size: %{public}zu", taskQueue_.size());
    }
    taskCv_.notify_one();
}

void AsyncDropDispatcher::WorkerLoop()
{
    AVCODEC_LOGD("WorkerLoop started");
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
                AVCODEC_LOGD("Executing task, remaining queue size (before pop): %{public}zu", taskQueue_.size());
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
        }
        if (task) {
            task();
        }
    }
    AVCODEC_LOGD("WorkerLoop exited");
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
