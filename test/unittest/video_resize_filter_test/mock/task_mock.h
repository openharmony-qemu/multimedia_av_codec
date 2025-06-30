/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef TASK_MOCK_H
#define TASK_MOCK_H

#include <gmock/gmock.h>
#include "video_resize_filter.h"

namespace OHOS {
namespace Media {
class MockTask : public Task {
public:
    MockTask(const std::string& name, const std::string& groupId, TaskType type, TaskPriority priority, bool singleLoop)
        :Task(name, groupId, type, priority, singleLoop) {}
    ~MockTask() = default;
 
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
    static void SleepInTask(unsigned ms) {}
    void SetEnableStateChangeLog(bool enable) {}
    void UpdateThreadPriority(const uint32_t newPriority, const std::string &strBundleName) {}
};
}  // namespace Media
}  // namespace OHOS

#endif // TASK_MOCK_H