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

    dispatcher.SubmitTask([&]() {
        executed = true;
    });

    // Wait for async execution
    auto start = std::chrono::steady_clock::now();
    while (!executed && (std::chrono::steady_clock::now() - start) < std::chrono::seconds(2)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    constexpr int NUM_TASKS = 50;

    for (int i = 0; i < NUM_TASKS; ++i) {
        dispatcher.SubmitTask([&]() {
            counter++;
        });
    }

    auto start = std::chrono::steady_clock::now();
    while (counter < NUM_TASKS &&
           (std::chrono::steady_clock::now() - start) < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(counter.load(), NUM_TASKS);
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
        for (int i = 0; i < 10; ++i) {
            dispatcher.SubmitTask([&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                counter++;
            });
        }
        // Destructor should wait for all tasks to finish
    }
    EXPECT_EQ(counter.load(), 10);
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

    for (int i = 0; i < 20; ++i) {
        dispatcher.SubmitTask([&, i]() {
            std::lock_guard<std::mutex> lock(mtx);
            results.push_back(i);
        });
    }

    // Wait for completion
    auto start = std::chrono::steady_clock::now();
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (static_cast<int>(results.size()) >= 20) {
                break;
            }
        }
        if ((std::chrono::steady_clock::now() - start) > std::chrono::seconds(5)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::lock_guard<std::mutex> lock(mtx);
    EXPECT_EQ(static_cast<int>(results.size()), 20);
    // Verify FIFO order
    for (int i = 0; i < static_cast<int>(results.size()); ++i) {
        EXPECT_EQ(results[i], i);
    }
}

