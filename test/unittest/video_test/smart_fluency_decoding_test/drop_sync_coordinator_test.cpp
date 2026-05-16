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
#include <thread>
#include <vector>
#include <atomic>
#include "drop_sync_coordinator.h"

namespace {
constexpr int32_t COORDINATOR_CAPACITY = 1024;
constexpr double DEFAULT_TOLERANCE = 0.001;
}

using namespace testing::ext;
using namespace OHOS::MediaAVCodec::SFD;

class DropSyncCoordinatorTest : public testing::Test {
protected:
    DropSyncCoordinator tracker_;

    void SetUp() override
    {
        tracker_.Reset();
    }

    void TearDown() override
    {
        tracker_.Reset();
    }
};

/**
 * @tc.name: RecordPreDroppedFrame_BasicInOrder_001
 * @tc.desc: Basic in-order read and write operations for dropped frame coordination.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, RecordPreDroppedFrame_BasicInOrder_001, TestSize.Level1)
{
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(10, 1.0)); // 10: pts
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(20, 1.0)); // 20: pts
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(30, 1.0)); // 30: pts

    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(25), 2); // 25: query pts
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(35), 1); // 35: query pts
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(40), 0);
}

/**
 * @tc.name: RecordPreDroppedFrame_ComplexGOPSequence_002
 * @tc.desc: Test queue behavior with a complex GOP sequence involving out-of-order PTS.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, RecordPreDroppedFrame_ComplexGOPSequence_002, TestSize.Level1)
{
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(2, 1.0)); // 2: pts
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(3, 1.0)); // 3: pts
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(4), 2);

    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(5, 1.0)); // 5: pts
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(6, 1.0)); // 6: pts
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(7), 2);
}

/**
 * @tc.name: RecordPreDroppedFrame_QueueOverflowProtection_003
 * @tc.desc: Ensure that the ring buffer protects against overflow when capacity is reached.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, RecordPreDroppedFrame_QueueOverflowProtection_003, TestSize.Level1)
{
    // 1. Fill the queue up to its true capacity (1024)
    for (int i = 0; i < COORDINATOR_CAPACITY; ++i) {
        EXPECT_TRUE(tracker_.RecordPreDroppedFrame(i, 1.0));
    }

    // 2. Queue is full; the 1025th frame (index 1024) must be rejected
    EXPECT_FALSE(tracker_.RecordPreDroppedFrame(1024, 1.0)); // 1024: overflow index

    // 3. Consume half of the data (512 frames) to free space
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(512), 512); // 512: half capacity

    // 4. Confirm new data can be written again after freeing space
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(1025, 1.0)); // 1025: recovery index
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(1026, 1.0)); // 1026: recovery index
}

/**
 * @tc.name: GetAndConsume_StressTest_004
 * @tc.desc: Single-Producer Single-Consumer pressure testing in a multi-threaded scenario.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, GetAndConsume_StressTest_004, TestSize.Level1)
{
    constexpr int totalFrames = 100000; // 100000: total stress frames
    std::atomic<int> totalConsumed{0};

    std::thread producer([this]() {
        for (int i = 0; i < totalFrames; ++i) {
            while (!tracker_.RecordPreDroppedFrame(i, 1.0)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([this, &totalConsumed]() {
        int currentQueryPts = 0;
        while (totalConsumed < totalFrames) {
            currentQueryPts += 10; // 10: query step
            int count = tracker_.GetAndConsumePreDroppedCount(currentQueryPts);
            totalConsumed += count;
            if (count == 0) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(totalConsumed.load(), totalFrames);
}

// ==========================================
// EMA Feedback Tests
// ==========================================


HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_EmaIncrease_006, TestSize.Level1)
{
    EXPECT_NEAR(tracker_.GetEmaRatio(), 1.0, DEFAULT_TOLERANCE);

    for (int i = 0; i < 10; ++i) { // 10: iterations
        tracker_.UpdateFeedback(true, 1.0);
    }

    EXPECT_NEAR(tracker_.GetEmaRatio(), 1.0, DEFAULT_TOLERANCE);
}

HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_PreDroppedCount_007, TestSize.Level1)
{
    tracker_.RecordPreDroppedFrame(100, 1.0); // 100: pts
    EXPECT_LT(tracker_.GetEmaRatio(), 1.0);
}

HWTEST_F(DropSyncCoordinatorTest, Reset_ClearsEma_008, TestSize.Level1)
{
    tracker_.UpdateFeedback(false, 1.0);
    EXPECT_LT(tracker_.GetEmaRatio(), 1.0);

    tracker_.Reset();
    EXPECT_NEAR(tracker_.GetEmaRatio(), 1.0, DEFAULT_TOLERANCE);
}

HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_SpeedAffectsAlpha_009, TestSize.Level1)
{
    DropSyncCoordinator fast;
    DropSyncCoordinator slow;

    for (int i = 0; i < 5; ++i) { // 5: iterations
        fast.UpdateFeedback(false, 3.0); // 3.0: high speed
        slow.UpdateFeedback(false, 0.5); // 0.5: low speed
    }

    EXPECT_LT(fast.GetEmaRatio(), slow.GetEmaRatio());
}

/**
 * @tc.name: GetAndConsume_EmptyQueueReturnsZero_010
 * @tc.desc: Checking count from an empty queue should safely return zero.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, GetAndConsume_EmptyQueueReturnsZero_010, TestSize.Level1)
{
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(100), 0); // 100: query pts
}

/**
 * @tc.name: GetAndConsume_FuturePts_011
 * @tc.desc: Verify frames with PTS >= currentPts are skipped during consumption.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, GetAndConsume_FuturePts_011, TestSize.Level1)
{
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(100, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(200, 1.0));

    // Query with PTS 50: both frames have PTS >= 50, so none should be consumed
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(50), 0);
    // Frames are still there
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(300), 2);
}

/**
 * @tc.name: GetAndConsume_AlreadyConsumed_012
 * @tc.desc: Verify already-consumed frames are skipped correctly.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, GetAndConsume_AlreadyConsumed_012, TestSize.Level1)
{
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(100, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(200, 1.0));

    // Consume first frame
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(150), 1);
    // Consume second frame — first should be skipped (already consumed)
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(300), 1);
}