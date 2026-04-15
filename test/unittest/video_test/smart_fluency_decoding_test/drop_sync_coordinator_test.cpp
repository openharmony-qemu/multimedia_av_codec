#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "drop_sync_coordinator.h"

using namespace testing::ext;

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
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(10));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(20));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(30));

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
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(2));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(3));
    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(4), 2);

    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(5));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(6));
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
        EXPECT_TRUE(tracker_.RecordPreDroppedFrame(i));
    }
    EXPECT_FALSE(tracker_.RecordPreDroppedFrame(129));

    EXPECT_EQ(tracker_.GetAndConsumePreDroppedCount(64), 64);
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(130));
    EXPECT_TRUE(tracker_.RecordPreDroppedFrame(131));
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
            while (!tracker_.RecordPreDroppedFrame(i)) {
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
    tracker_.UpdateFeedback(0, false, 0.3, 1.0);
    EXPECT_NEAR(tracker_.GetTargetRetentionRatio(), 0.3, 0.001);

    tracker_.UpdateFeedback(0, false, 0.7, 1.0);
    EXPECT_NEAR(tracker_.GetTargetRetentionRatio(), 0.7, 0.001);
}

/**
 * @tc.name: UpdateFeedback_EmaIncrease_006
 * @tc.desc: UpdateFeedback should increase EMA when frames are dropped.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_EmaIncrease_006, TestSize.Level1)
{
    EXPECT_NEAR(tracker_.GetCurrentEma(), 0.0, 0.001);

    // Simulate several dropped frames
    for (int i = 0; i < 10; ++i) {
        tracker_.UpdateFeedback(0, true, 0.5, 1.0);
    }

    // EMA should have increased toward 1.0
    EXPECT_GT(tracker_.GetCurrentEma(), 0.5);
}

/**
 * @tc.name: UpdateFeedback_PreDroppedCount_007
 * @tc.desc: Check EMA adjustment when pre-dropped frames are reported.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_PreDroppedCount_007, TestSize.Level1)
{
    // 5 pre-dropped frames all counted as dropped (value=1.0)
    tracker_.UpdateFeedback(5, false, 0.5, 1.0);
    EXPECT_GT(tracker_.GetCurrentEma(), 0.0);
}

/**
 * @tc.name: Reset_ClearsEma_008
 * @tc.desc: Verify that Reset correctly clears the EMA values.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, Reset_ClearsEma_008, TestSize.Level1)
{
    tracker_.UpdateFeedback(0, true, 0.5, 1.0);
    EXPECT_GT(tracker_.GetCurrentEma(), 0.0);

    tracker_.Reset();
    EXPECT_NEAR(tracker_.GetCurrentEma(), 0.0, 0.001);
}

/**
 * @tc.name: UpdateFeedback_SpeedAffectsAlpha_009
 * @tc.desc: Validate that current decoding speed factors into the alpha of EMA calculation.
 * @tc.type: FUNC
 */
HWTEST_F(DropSyncCoordinatorTest, UpdateFeedback_SpeedAffectsAlpha_009, TestSize.Level1)
{
    DropSyncCoordinator fast;
    DropSyncCoordinator slow;

    // Same drop events but different speeds
    for (int i = 0; i < 5; ++i) {
        fast.UpdateFeedback(0, true, 0.5, 3.0);
        slow.UpdateFeedback(0, true, 0.5, 0.5);
    }

    // Higher speed = higher alpha = faster convergence to 1.0
    EXPECT_GT(fast.GetCurrentEma(), slow.GetCurrentEma());
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