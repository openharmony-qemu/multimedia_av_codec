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
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(10, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(20, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(30, 1.0));

    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(25), 2);
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(35), 1);
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(40), 0);
}

/**
 * @tc.name: RecordPreDroppedFrame_ComplexGOPSequence_002
 * @tc.desc: Test queue behavior with a complex GOP sequence involving out-of-order PTS.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, RecordPreDroppedFrame_ComplexGOPSequence_002, TestSize.Level1)
{
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(2, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(3, 1.0));
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(4), 2);

    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(5, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(6, 1.0));
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(7), 2);
}

/**
 * @tc.name: RecordPreDroppedFrame_QueueOverflowProtection_003
 * @tc.desc: Ensure that the ring buffer protects against overflow when capacity is reached.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, RecordPreDroppedFrame_QueueOverflowProtection_003, TestSize.Level1)
{
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(tracker_.RecordPreDroppedFrame(i, 1.0));
    }
    EXPECT_FALSE(tracker_.RecordPreDroppedFrame(129, 1.0));

    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(64), 64);
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(130, 1.0));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(131, 1.0));
}

/**
 * @tc.name: GetAndConsume_StressTest_004
 * @tc.desc: Single-Producer Single-Consumer pressure testing in a multi-threaded scenario.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, GetAndConsume_StressTest_004, TestSize.Level1)
{
    constexpr int TOTAL_FRAMES = 100000;
    std::atomic<int> total_consumed{0};

    std::thread producer([&]() {
        for (int i = 0; i < TOTAL_FRAMES; ++i) {
            while (!tracker_.RecordPreDroppedFrame(i, 1.0)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        int current_query_pts = 0;
        while (total_consumed < TOTAL_FRAMES) {
            current_query_pts += 10;
            int count = tracker_.GetAndConsumePreDroppedCount(current_query_pts);
            total_consumed += count;
            if (count == 0) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(total_consumed.load(), TOTAL_FRAMES);
}

// ==========================================
// EMA Feedback Tests
// ==========================================

/**
 * @tc.name: UpdateFeedback_TargetRatio_005
 * @tc.desc: Validate that UpdateFeedback correctly updates and stores the target ratio.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_TargetRatio_005, TestSize.Level1)
{
    tracker_.UpdateFeedback(false, 0.3, 1.0);
    EXPECT_NEAR(tracker_.GetTargetRatio(), 0.3, 0.001);

    tracker_.UpdateFeedback(false, 0.7, 1.0);
    EXPECT_NEAR(tracker_.GetTargetRatio(), 0.7, 0.001);
}

HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_EmaIncrease_006, TestSize.Level1)
{
    EXPECT_NEAR(tracker_.GetEmaRatio(), 1.0, 0.001);

    for (int i = 0; i < 10; ++i) {
        tracker_.UpdateFeedback(true, 0.5, 1.0);
    }

    EXPECT_NEAR(tracker_.GetEmaRatio(), 1.0, 0.001);
}

HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_PreDroppedCount_007, TestSize.Level1)
{
    tracker_.RecordPreDroppedFrame(100, 1.0);
    EXPECT_LT(tracker_.GetEmaRatio(), 1.0);
}

HWTEST_F(DropSyncCoordinatorTest, Reset_ClearsEma_008, TestSize.Level1)
{
    tracker_.UpdateFeedback(false, 0.5, 1.0);
    EXPECT_LT(tracker_.GetEmaRatio(), 1.0);

    tracker_.Reset();
    EXPECT_NEAR(tracker_.GetEmaRatio(), 1.0, 0.001);
}

HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_SpeedAffectsAlpha_009, TestSize.Level1)
{
    DropSyncCoordinator fast;
    DropSyncCoordinator slow;

    for (int i = 0; i < 5; ++i) {
        fast.UpdateFeedback(false, 0.5, 3.0);
        slow.UpdateFeedback(false, 0.5, 0.5);
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
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(100), 0);
}