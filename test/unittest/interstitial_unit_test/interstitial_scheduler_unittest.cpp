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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "interstitial_scheduler.h"
#include "interstitial_vod_strategies.h"
#include "interstitial_live_strategies.h"
#include "mock/mock_media_sync_manager.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

class InterstitialSchedulerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    std::shared_ptr<InterstitialScheduler> scheduler_;
    std::shared_ptr<Pipeline::MockMediaSyncManager> mockSyncMgr_;
};

void InterstitialSchedulerUnitTest::SetUpTestCase(void) {}
void InterstitialSchedulerUnitTest::TearDownTestCase(void) {}

void InterstitialSchedulerUnitTest::SetUp(void)
{
    scheduler_ = std::make_shared<InterstitialScheduler>();
    mockSyncMgr_ = std::make_shared<Pipeline::MockMediaSyncManager>();
}

void InterstitialSchedulerUnitTest::TearDown(void)
{
    scheduler_ = nullptr;
    mockSyncMgr_ = nullptr;
}

// ==================== CollectEvent ====================

/**
 * @tc.name  : Test CollectEvent
 * @tc.number: Collect_001
 * @tc.desc  : Test CollectEvent with null metadata
 */
HWTEST_F(InterstitialSchedulerUnitTest, Collect_001, TestSize.Level1)
{
    scheduler_->CollectEvent(nullptr);
    EXPECT_TRUE(scheduler_->pendingEvents_.empty());
}

/**
 * @tc.name  : Test CollectEvent
 * @tc.number: Collect_002
 * @tc.desc  : Test CollectEvent with duplicate id
 */
HWTEST_F(InterstitialSchedulerUnitTest, Collect_002, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 1000;
    scheduler_->CollectEvent(meta);
    scheduler_->CollectEvent(meta);
    EXPECT_EQ(scheduler_->pendingEvents_.size(), 1u);
}

/**
 * @tc.name  : Test CollectEvent
 * @tc.number: Collect_003
 * @tc.desc  : Test CollectEvent with valid metadata
 */
HWTEST_F(InterstitialSchedulerUnitTest, Collect_003, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);
    EXPECT_EQ(scheduler_->pendingEvents_.size(), 1u);
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test CollectEvent with start=0 immediate notify
 * @tc.number: Collect_004
 * @tc.desc  : Test CollectEvent immediately notifies when start <= NOTIFY_ADVANCE_MS
 */
HWTEST_F(InterstitialSchedulerUnitTest, Collect_004, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_preroll";
    meta->start = 0;

    bool callbackFired = false;
    std::string receivedId;
    scheduler_->SetNotifyCallback([&](const std::shared_ptr<MediaAVCodec::AVTimedMetaData>& m) {
        callbackFired = true;
        receivedId = m->id;
    });

    scheduler_->CollectEvent(meta);
    EXPECT_TRUE(callbackFired);
    EXPECT_EQ(receivedId, "event_preroll");
    EXPECT_TRUE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test CollectEvent with start=1500 immediate notify
 * @tc.number: Collect_005
 * @tc.desc  : Test CollectEvent immediately notifies when start <= NOTIFY_ADVANCE_MS (2000)
 */
HWTEST_F(InterstitialSchedulerUnitTest, Collect_005, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_early";
    meta->start = 1500;

    bool callbackFired = false;
    scheduler_->SetNotifyCallback([&](const std::shared_ptr<MediaAVCodec::AVTimedMetaData>&) {
        callbackFired = true;
    });

    scheduler_->CollectEvent(meta);
    EXPECT_TRUE(callbackFired);
    EXPECT_TRUE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test CollectEvent with start=5000 no immediate notify
 * @tc.number: Collect_006
 * @tc.desc  : Test CollectEvent does not immediately notify when start > NOTIFY_ADVANCE_MS
 */
HWTEST_F(InterstitialSchedulerUnitTest, Collect_006, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_late";
    meta->start = 5000;

    bool callbackFired = false;
    scheduler_->SetNotifyCallback([&](const std::shared_ptr<MediaAVCodec::AVTimedMetaData>&) {
        callbackFired = true;
    });

    scheduler_->CollectEvent(meta);
    EXPECT_FALSE(callbackFired);
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

// ==================== OnPlaybackTick ====================

/**
 * @tc.name  : Test OnPlaybackTick
 * @tc.number: Tick_001
 * @tc.desc  : Test OnPlaybackTick with null syncCenter
 */
HWTEST_F(InterstitialSchedulerUnitTest, Tick_001, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);
    scheduler_->OnPlaybackTick();
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test OnPlaybackTick
 * @tc.number: Tick_002
 * @tc.desc  : Test OnPlaybackTick with event due (2s advance)
 */
HWTEST_F(InterstitialSchedulerUnitTest, Tick_002, TestSize.Level1)
{
    scheduler_->SetSyncCenter(mockSyncMgr_);
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);

    bool callbackFired = false;
    scheduler_->SetNotifyCallback([&callbackFired](const std::shared_ptr<MediaAVCodec::AVTimedMetaData>&) {
        callbackFired = true;
    });

    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(3500000));
    scheduler_->OnPlaybackTick();
    EXPECT_TRUE(callbackFired);
    EXPECT_TRUE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test OnPlaybackTick
 * @tc.number: Tick_003
 * @tc.desc  : Test OnPlaybackTick with event not due
 */
HWTEST_F(InterstitialSchedulerUnitTest, Tick_003, TestSize.Level1)
{
    scheduler_->SetSyncCenter(mockSyncMgr_);
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 10000;
    scheduler_->CollectEvent(meta);

    bool callbackFired = false;
    scheduler_->SetNotifyCallback([&callbackFired](const std::shared_ptr<MediaAVCodec::AVTimedMetaData>&) {
        callbackFired = true;
    });

    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(5000000));
    scheduler_->OnPlaybackTick();
    EXPECT_FALSE(callbackFired);
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test OnPlaybackTick with Live strategy TrimExpiredEvents
 * @tc.number: Tick_004
 * @tc.desc  : Test OnPlaybackTick with Live strategy trims expired events
 */
HWTEST_F(InterstitialSchedulerUnitTest, Tick_004, TestSize.Level1)
{
    scheduler_->SetSyncCenter(mockSyncMgr_);
    auto strategy = std::make_shared<LiveScheduleStrategy>();
    scheduler_->SetScheduleStrategy(strategy);

    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 3000;
    scheduler_->CollectEvent(meta);
    scheduler_->pendingEvents_[0].notified = true;

    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(5000000));
    scheduler_->OnPlaybackTick();
    EXPECT_TRUE(scheduler_->pendingEvents_.empty());
}

/**
 * @tc.name  : Test OnPlaybackTick with currentPosMs <= 0
 * @tc.number: Tick_005
 * @tc.desc  : Test OnPlaybackTick returns early when currentPosMs <= 0
 */
HWTEST_F(InterstitialSchedulerUnitTest, Tick_005, TestSize.Level1)
{
    scheduler_->SetSyncCenter(mockSyncMgr_);
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);

    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(0));
    scheduler_->OnPlaybackTick();
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

// ==================== OnSeek ====================

/**
 * @tc.name  : Test OnSeek
 * @tc.number: Seek_001
 * @tc.desc  : Test OnSeek with strategy
 */
HWTEST_F(InterstitialSchedulerUnitTest, Seek_001, TestSize.Level1)
{
    auto strategy = std::make_shared<VodScheduleStrategy>();
    scheduler_->SetScheduleStrategy(strategy);
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);
    scheduler_->OnSeek(3000);
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

/**
 * @tc.name  : Test OnSeek
 * @tc.number: Seek_002
 * @tc.desc  : Test OnSeek without strategy
 */
HWTEST_F(InterstitialSchedulerUnitTest, Seek_002, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);
    scheduler_->OnSeek(3000);
    EXPECT_FALSE(scheduler_->pendingEvents_[0].notified);
}

// ==================== OnStop ====================

/**
 * @tc.name  : Test OnStop
 * @tc.number: Stop_001
 * @tc.desc  : Test OnStop clears events
 */
HWTEST_F(InterstitialSchedulerUnitTest, Stop_001, TestSize.Level1)
{
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 5000;
    scheduler_->CollectEvent(meta);
    scheduler_->OnStop();
    EXPECT_TRUE(scheduler_->pendingEvents_.empty());
}

// ==================== GetScheduleStrategy ====================

/**
 * @tc.name  : Test GetScheduleStrategy
 * @tc.number: GetStrat_001
 * @tc.desc  : Test GetScheduleStrategy
 */
HWTEST_F(InterstitialSchedulerUnitTest, GetStrat_001, TestSize.Level1)
{
    auto strategy = std::make_shared<VodScheduleStrategy>();
    scheduler_->SetScheduleStrategy(strategy);
    EXPECT_EQ(scheduler_->GetScheduleStrategy(), strategy);
}

} // namespace Media
} // namespace OHOS
