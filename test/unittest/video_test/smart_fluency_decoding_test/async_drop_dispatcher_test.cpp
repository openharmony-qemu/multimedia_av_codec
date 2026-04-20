/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "async_drop_dispatcher.h"

namespace {
constexpr int32_t NUM_TASKS_STRESS = 50;
constexpr int32_t NUM_TASKS_FIFO = 20;
constexpr int32_t NUM_TASKS_DRAIN = 10;
}

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class AsyncDropDispatcherTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: SubmitTask_Basic_001
 * @tc.desc: Validate that a single task submitted to the dispatcher is executed asynchronously.
 * @tc.type: FUNC
 */
HWTEST_F(AsyncDropDispatcherTest, SubmitTask_Basic_001, TestSize.Level1)
{
    AsyncDropDispatcher dispatcher;
    std::atomic<bool> executed{false};

    dispatcher.SubmitTask([&executed]() {
        executed = true;
    });

    // Wait for async execution
    auto start = std::chrono::steady_clock::now();
    while (!executed && (std::chrono::steady_clock::now() - start) < std::chrono::seconds(2)) { // 2: seconds timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10: ms sleep
    }
    EXPECT_TRUE(executed.load());
}

/**
 * @tc.name: SubmitTask_Multiple_002
 * @tc.desc: Validate that multiple tasks submitted are all executed correctly.
 * @tc.type: FUNC
 */
HWTEST_F(AsyncDropDispatcherTest, SubmitTask_Multiple_002, TestSize.Level1)
{
    AsyncDropDispatcher dispatcher;
    std::atomic<int> counter{0};

    for (int i = 0; i < NUM_TASKS_STRESS; ++i) {
        dispatcher.SubmitTask([&counter]() {
            counter++;
        });
    }

    auto start = std::chrono::steady_clock::now();
    while (counter < NUM_TASKS_STRESS &&
           (std::chrono::steady_clock::now() - start) < std::chrono::seconds(5)) { // 5: seconds timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10: ms sleep
    }
    EXPECT_EQ(counter.load(), NUM_TASKS_STRESS);
}

/**
 * @tc.name: Destructor_DrainsTasks_003
 * @tc.desc: Verify that the destructor waits for all pending tasks to finish before destroying.
 * @tc.type: FUNC
 */
HWTEST_F(AsyncDropDispatcherTest, Destructor_DrainsTasks_003, TestSize.Level1)
{
    std::atomic<int> counter{0};
    {
        AsyncDropDispatcher dispatcher;
        for (int i = 0; i < NUM_TASKS_DRAIN; ++i) {
            dispatcher.SubmitTask([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 5: ms sleep
                counter++;
            });
        }
        // Destructor should wait for all tasks to finish
    }
    EXPECT_EQ(counter.load(), NUM_TASKS_DRAIN);
}

/**
 * @tc.name: SubmitTask_TaskOrdering_004
 * @tc.desc: Ensure that tasks submitted are executed in FIFO (First-In-First-Out) order.
 * @tc.type: FUNC
 */
HWTEST_F(AsyncDropDispatcherTest, SubmitTask_TaskOrdering_004, TestSize.Level1)
{
    AsyncDropDispatcher dispatcher;
    std::vector<int> results;
    std::mutex mtx;

    for (int i = 0; i < NUM_TASKS_FIFO; ++i) {
        dispatcher.SubmitTask([&mtx, &results, i]() {
            std::lock_guard<std::mutex> lock(mtx);
            results.push_back(i);
        });
    }

    // Wait for completion
    auto start = std::chrono::steady_clock::now();
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (static_cast<int>(results.size()) >= NUM_TASKS_FIFO) {
                break;
            }
        }
        if ((std::chrono::steady_clock::now() - start) > std::chrono::seconds(5)) { // 5: seconds timeout
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10: ms sleep
    }

    std::lock_guard<std::mutex> lock(mtx);
    EXPECT_EQ(static_cast<int>(results.size()), NUM_TASKS_FIFO);
    // Verify FIFO order
    for (int i = 0; i < static_cast<int>(results.size()); ++i) {
        EXPECT_EQ(results[i], i);
    }
}
