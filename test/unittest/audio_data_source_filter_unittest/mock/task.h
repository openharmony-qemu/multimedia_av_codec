/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_FOUNDATION_OSAL_TASK_H
#define HISTREAMER_FOUNDATION_OSAL_TASK_H

#include <functional>
#include <string>
#include <memory>
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {

enum class TaskPriority : int {
    LOW,
    NORMAL,
    MIDDLE,
    HIGH,
    HIGHEST,
};

enum class TaskType : int {
    GLOBAL,
    VIDEO,
    AUDIO,
    DEMUXER,
    DECODER,
    SUBTITLE,
    SINGLETON,
};

class TaskInner;

class Task {
public:
    explicit Task(const std::string& name) {};
    virtual ~Task() = default;
    MOCK_METHOD(void, Start, (), ());
    MOCK_METHOD(void, Stop, (), ());
    MOCK_METHOD(void, StopAsync, (), ());
    MOCK_METHOD(void, Pause, (), ());
    MOCK_METHOD(void, PauseAsync, (), ());
    MOCK_METHOD(void, RegisterJob, (const std::function<int64_t()>& job), ());
    MOCK_METHOD(void, SubmitJobOnce, (const std::function<void()>& job, int64_t delayUs, bool wait), ());
    MOCK_METHOD(void, SubmitJob, (const std::function<void()>& job, int64_t delayUs, bool wait), ());
    MOCK_METHOD(bool, IsTaskRunning, (), ());
    MOCK_METHOD(void, UpdateDelayTime, (int64_t delayUs), ());

private:
    std::shared_ptr<TaskInner> taskInner_;
};
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_FOUNDATION_OSAL_TASK_H

