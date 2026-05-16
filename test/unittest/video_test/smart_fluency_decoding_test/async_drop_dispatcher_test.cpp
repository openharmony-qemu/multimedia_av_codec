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
#include <future>
#include "async_drop_dispatcher.h"

namespace {
constexpr int32_t NUM_TASKS_STRESS = 50;
constexpr int32_t NUM_TASKS_FIFO = 20;
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
    std::promise<void> taskDone;
    auto future = taskDone.get_future();

    dispatcher.SubmitTask([&taskDone]() {
        taskDone.set_value();
    });

    // Wait for async execution with 1s timeout to avoid deadlocks
    auto status = future.wait_for(std::chrono::seconds(1));
    EXPECT_EQ(status, std::future_status::ready);
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
    std::mutex mtx;
    std::condition_variable cv;
    for (int i = 0; i < NUM_TASKS_STRESS; ++i) {
        dispatcher.SubmitTask([&counter, &cv]() {
            counter++;
            if (counter == NUM_TASKS_STRESS) {
                cv.notify_one();
            }
        });
    }
    std::unique_lock<std::mutex> lock(mtx);
    bool success = cv.wait_for(lock, std::chrono::seconds(2), [&counter]() {
        return counter == NUM_TASKS_STRESS;
    });
    EXPECT_TRUE(success);
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
    std::promise<void> startPromise;
    std::future<void> startFuture = startPromise.get_future();
    {
        AsyncDropDispatcher dispatcher;
        // Submit a "barrier" task that blocks the worker thread
        dispatcher.SubmitTask([&startFuture, &counter]() {
            startFuture.wait(); // Wait until startPromise is set
            counter++;
        });
        // Submit 9 more tasks - they will be queued since the worker is blocked
        for (int i = 0; i < 9; ++i) {
            dispatcher.SubmitTask([&counter]() {
                counter++;
            });
        }
        // Release the barrier and immediately let dispatcher go out of scope
        startPromise.set_value();
    } // Destructor triggers here and MUST join the worker thread after draining all 10 tasks
    // Destructor should have blocked until all tasks finished
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
    std::condition_variable cv;
    for (int i = 0; i < NUM_TASKS_FIFO; ++i) {
        dispatcher.SubmitTask([&mtx, &results, &cv, i]() {
            {
                std::lock_guard<std::mutex> lock(mtx);
                results.push_back(i);
            }
            cv.notify_one();
        });
    }
    {
        std::unique_lock<std::mutex> lock(mtx);
        bool success = cv.wait_for(lock, std::chrono::seconds(5), [&results]() {
            return static_cast<int>(results.size()) == NUM_TASKS_FIFO;
        });
        EXPECT_TRUE(success);
    }
    std::lock_guard<std::mutex> verifyLock(mtx);
    ASSERT_EQ(static_cast<int>(results.size()), NUM_TASKS_FIFO);
    for (int i = 0; i < static_cast<int>(results.size()); ++i) {
        EXPECT_EQ(results[i], i);
    }
}

/**
 * @tc.name: SubmitTask_NullTask_005
 * @tc.desc: Verify SubmitTask returns early without crash when a null task is provided.
 * @tc.type: FUNC
 */
HWTEST_F(AsyncDropDispatcherTest, SubmitTask_NullTask_005, TestSize.Level1)
{
    AsyncDropDispatcher dispatcher;
    dispatcher.SubmitTask(nullptr);
    dispatcher.SubmitTask(std::function<void()>());
    // No crash = success
    EXPECT_TRUE(true);
}

/**
 * @tc.name: SubmitTask_StoppedDispatcher_006
 * @tc.desc: Verify SubmitTask rejects tasks after the dispatcher has been stopped.
 * @tc.type: FUNC
 */
HWTEST_F(AsyncDropDispatcherTest, SubmitTask_StoppedDispatcher_006, TestSize.Level1)
{
    AsyncDropDispatcher dispatcher;
    std::atomic<int> counter{0};
    std::atomic<bool> taskStarted{false};
    std::promise<void> blocker;
    auto future = blocker.get_future();

    // Submit a blocking task to hold the worker thread
    dispatcher.SubmitTask([&]() {
        taskStarted.store(true);
        future.wait();
        counter++;
    });

    // Wait for the blocking task to start executing
    while (!taskStarted.load()) {
        std::this_thread::yield();
    }

    // Now stop the dispatcher
    dispatcher.isWorking_.store(false);

    // Submit a task — should be rejected since isWorking_ is false
    dispatcher.SubmitTask([&counter]() { counter++; });

    // Release the blocker to let the worker thread exit
    blocker.set_value();

    // Wait for the worker to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Only the first (blocking) task should have executed
    EXPECT_EQ(counter.load(), 1);
}
