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

#include "interstitial_controller_unittest.h"
#include "interstitial_scheduler.h"
#include "common/media_source.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void InterstitialControllerUnitTest::SetUpTestCase(void) {}
void InterstitialControllerUnitTest::TearDownTestCase(void) {}

void InterstitialControllerUnitTest::SetUp(void)
{
    controller_ = std::make_shared<InterstitialController>();
    mockDemuxer_ = std::make_shared<MockMediaDemuxer>();
    mockSyncMgr_ = std::make_shared<Pipeline::MockMediaSyncManager>();

    ON_CALL(*mockSyncMgr_, GetPlaybackRate()).WillByDefault(Return(1.0f));
    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(0));
}

void InterstitialControllerUnitTest::TearDown(void)
{
    controller_ = nullptr;
    mockDemuxer_ = nullptr;
    mockSyncMgr_ = nullptr;
}

// ==================== Init ====================

/**
 * @tc.name  : Test Init
 * @tc.number: Init_001
 * @tc.desc  : Test Init with null mainDemuxer
 */
HWTEST_F(InterstitialControllerUnitTest, Init_001, TestSize.Level1)
{
    auto ret = controller_->Init(nullptr, mockSyncMgr_);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test Init
 * @tc.number: Init_002
 * @tc.desc  : Test Init with valid params
 */
HWTEST_F(InterstitialControllerUnitTest, Init_002, TestSize.Level1)
{
    auto ret = controller_->Init(mockDemuxer_, mockSyncMgr_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test Init
 * @tc.number: Init_003
 * @tc.desc  : Test Init with null syncMgr
 */
HWTEST_F(InterstitialControllerUnitTest, Init_003, TestSize.Level1)
{
    auto ret = controller_->Init(mockDemuxer_, nullptr);
    EXPECT_EQ(ret, Status::OK);
}

// ==================== SetLiveSource ====================

/**
 * @tc.name  : Test SetLiveSource
 * @tc.number: SetLiveSource_001
 * @tc.desc  : Test SetLiveSource with true
 */
HWTEST_F(InterstitialControllerUnitTest, SetLiveSource_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(true);
    EXPECT_TRUE(controller_->IsLiveSource());
    auto strategy = controller_->GetScheduleStrategy();
    EXPECT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetName(), "LiveSchedule");
}

/**
 * @tc.name  : Test SetLiveSource
 * @tc.number: SetLiveSource_002
 * @tc.desc  : Test SetLiveSource with false
 */
HWTEST_F(InterstitialControllerUnitTest, SetLiveSource_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    EXPECT_FALSE(controller_->IsLiveSource());
    auto strategy = controller_->GetScheduleStrategy();
    EXPECT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetName(), "VodSchedule");
}

/**
 * @tc.name  : Test SetLiveSource
 * @tc.number: SetLiveSource_003
 * @tc.desc  : Test SetLiveSource propagates strategy to scheduler
 */
HWTEST_F(InterstitialControllerUnitTest, SetLiveSource_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto scheduler = std::make_shared<InterstitialScheduler>();
    controller_->SetScheduler(scheduler);
    controller_->SetLiveSource(false);
    EXPECT_EQ(scheduler->GetScheduleStrategy()->GetName(), "VodSchedule");
}

/**
 * @tc.name  : Test SetLiveSource with scheduler_ null skips propagation
 * @tc.number: SetLiveSource_004
 * @tc.desc  : Test SetLiveSource does not crash when scheduler_ is null
 */
HWTEST_F(InterstitialControllerUnitTest, SetLiveSource_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->scheduler_ = nullptr;
    controller_->SetLiveSource(true);
    EXPECT_TRUE(controller_->IsLiveSource());
    EXPECT_EQ(controller_->GetScheduleStrategy()->GetName(), "LiveSchedule");
}

// ==================== AddAdsMediaSource ====================

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: AddAds_001
 * @tc.desc  : Test AddAdsMediaSource with null source
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    std::string outId;
    auto ret = controller_->AddAdsMediaSource(nullptr, 1000, outId);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: AddAds_002
 * @tc.desc  : Test AddAdsMediaSource with valid source
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    auto ret = controller_->AddAdsMediaSource(source, 1000, outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(outId, "ad_0");
}

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: AddAds_003
 * @tc.desc  : Test AddAdsMediaSource with duplicate URI+startMs
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source, 1000, outId1);
    controller_->AddAdsMediaSource(source, 1000, outId2);
    EXPECT_EQ(outId1, outId2);
}

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: AddAds_004
 * @tc.desc  : Test AddAdsMediaSource with different startMs
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source, 1000, outId1);
    controller_->AddAdsMediaSource(source, 5000, outId2);
    EXPECT_NE(outId1, outId2);
}

/**
 * @tc.name  : Test AddAdsMediaSource with startMs=0 records pre-roll ad
 * @tc.number: AddAds_005
 * @tc.desc  : Test AddAdsMediaSource with startMs=0 records preRollAdId_ without triggering DoSwitch
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;

    auto ret = controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(outId, "ad_0");
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");
}

/**
 * @tc.name  : Test AddAdsMediaSource with startMs=0 in PLAYING state
 * @tc.number: AddAds_006
 * @tc.desc  : Test AddAdsMediaSource with startMs=0 does not record preRollAdId_ when already playing
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->playState_ = InterstitialPlayState::PLAYING;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    auto ret = controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::PLAYING);
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

/**
 * @tc.name  : Test multiple pre-roll ads only record first one
 * @tc.number: AddAds_007
 * @tc.desc  : Test AddAdsMediaSource with multiple startMs=0 ads only records the first one as preRollAdId_
 */
HWTEST_F(InterstitialControllerUnitTest, AddAds_007, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/preroll1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/preroll2.mp4");
    auto source3 = std::make_shared<Plugins::MediaSource>("http://example.com/preroll3.mp4");
    std::string outId1, outId2, outId3;

    auto ret1 = controller_->AddAdsMediaSource(source1, 0, outId1);
    EXPECT_EQ(ret1, Status::OK);
    EXPECT_EQ(outId1, "ad_0");
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");

    auto ret2 = controller_->AddAdsMediaSource(source2, 0, outId2);
    EXPECT_EQ(ret2, Status::OK);
    EXPECT_EQ(outId2, "ad_1");
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");

    auto ret3 = controller_->AddAdsMediaSource(source3, 0, outId3);
    EXPECT_EQ(ret3, Status::OK);
    EXPECT_EQ(outId3, "ad_2");
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");
}

// ==================== RemoveAdsMediaSource ====================

/**
 * @tc.name  : Test RemoveAdsMediaSource
 * @tc.number: RemoveAds_001
 * @tc.desc  : Test RemoveAdsMediaSource with valid id
 */
HWTEST_F(InterstitialControllerUnitTest, RemoveAds_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto ret = controller_->RemoveAdsMediaSource(outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(controller_->HasPendingEvents());
}

/**
 * @tc.name  : Test RemoveAdsMediaSource
 * @tc.number: RemoveAds_002
 * @tc.desc  : Test RemoveAdsMediaSource with non-existent id
 */
HWTEST_F(InterstitialControllerUnitTest, RemoveAds_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto ret = controller_->RemoveAdsMediaSource("non_existent_id");
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test RemoveAdsMediaSource in PLAYING state
 * @tc.number: RemoveAds_003
 * @tc.desc  : Test RemoveAdsMediaSource refuses to remove currently playing ad, returns OK
 */
HWTEST_F(InterstitialControllerUnitTest, RemoveAds_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId;
    auto ret = controller_->RemoveAdsMediaSource(outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(controller_->entries_.find(outId) != controller_->entries_.end());
}

// ==================== DisableAllAdsMediaSource ====================

/**
 * @tc.name  : Test DisableAllAdsMediaSource
 * @tc.number: Disable_001
 * @tc.desc  : Test DisableAllAdsMediaSource in IDLE state
 */
HWTEST_F(InterstitialControllerUnitTest, Disable_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto ret = controller_->DisableAllAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(controller_->IsAdsDisabled());
    EXPECT_FALSE(controller_->HasPendingEvents());
}

/**
 * @tc.name  : Test DisableAllAdsMediaSource in PLAYING state
 * @tc.number: Disable_002
 * @tc.desc  : Test DisableAllAdsMediaSource keeps currently playing ad
 */
HWTEST_F(InterstitialControllerUnitTest, Disable_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source1, 1000, outId1);
    controller_->AddAdsMediaSource(source2, 5000, outId2);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId1;
    auto ret = controller_->DisableAllAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(controller_->IsAdsDisabled());
    EXPECT_EQ(controller_->entries_.size(), 1u);
    EXPECT_TRUE(controller_->entries_.find(outId1) != controller_->entries_.end());
    EXPECT_TRUE(controller_->entries_.find(outId2) == controller_->entries_.end());
}

/**
 * @tc.name  : Test DisableAllAdsMediaSource PLAYING with currentAdId_ not in entries
 * @tc.number: Disable_003
 * @tc.desc  : Test DisableAllAdsMediaSource clears all when PLAYING and currentAdId_ not found
 */
HWTEST_F(InterstitialControllerUnitTest, Disable_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = "non_existent";
    auto ret = controller_->DisableAllAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(controller_->IsAdsDisabled());
    EXPECT_TRUE(controller_->entries_.empty());
}

// ==================== SkipCurrentAdsMediaSource ====================

/**
 * @tc.name  : Test SkipCurrentAdsMediaSource
 * @tc.number: Skip_001
 * @tc.desc  : Test SkipCurrentAdsMediaSource in IDLE state returns OK
 */
HWTEST_F(InterstitialControllerUnitTest, Skip_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto ret = controller_->SkipCurrentAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SkipCurrentAdsMediaSource in PLAYING state
 * @tc.number: Skip_002
 * @tc.desc  : Test SkipCurrentAdsMediaSource emits END(SKIPPED) and calls DoResume
 */
HWTEST_F(InterstitialControllerUnitTest, Skip_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId;
    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });
    auto ret = controller_->SkipCurrentAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(receivedEvent.type, AdsEventType::SKIPPED);
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test SkipCurrentAdsMediaSource with entry not found
 * @tc.number: Skip_003
 * @tc.desc  : Test SkipCurrentAdsMediaSource handles case where currentAdId_ not in entries
 */
HWTEST_F(InterstitialControllerUnitTest, Skip_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = "non_existent";
    auto ret = controller_->SkipCurrentAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

// ==================== OnStop ====================

/**
 * @tc.name  : Test OnStop
 * @tc.number: OnStop_001
 * @tc.desc  : Test OnStop in IDLE state
 */
HWTEST_F(InterstitialControllerUnitTest, OnStop_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->OnStop();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_FALSE(controller_->HasPendingEvents());
}

/**
 * @tc.name  : Test OnStop
 * @tc.number: OnStop_002
 * @tc.desc  : Test OnStop in PLAYING state
 */
HWTEST_F(InterstitialControllerUnitTest, OnStop_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->OnStop();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test OnStop resets resumePointMs
 * @tc.number: OnStop_003
 * @tc.desc  : Test OnStop resets resumePointMs to 0
 */
HWTEST_F(InterstitialControllerUnitTest, OnStop_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->resumePointMs_ = 5000;
    controller_->OnStop();
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

/**
 * @tc.name  : Test OnStop clears preRollAdId_
 * @tc.number: OnStop_004
 * @tc.desc  : Test OnStop clears preRollAdId_
 */
HWTEST_F(InterstitialControllerUnitTest, OnStop_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");
    controller_->OnStop();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

// ==================== OnSeek ====================

/**
 * @tc.name  : Test OnSeek
 * @tc.number: OnSeek_001
 * @tc.desc  : Test OnSeek with VOD strategy
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source1, 1000, outId1);
    controller_->AddAdsMediaSource(source2, 5000, outId2);
    controller_->OnSeek(3000);
    EXPECT_TRUE(controller_->entries_[outId1].played);
    EXPECT_FALSE(controller_->entries_[outId2].played);
}

/**
 * @tc.name  : Test OnSeek
 * @tc.number: OnSeek_002
 * @tc.desc  : Test OnSeek with Live strategy (no-op)
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(true);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->OnSeek(3000);
    EXPECT_FALSE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnSeek
 * @tc.number: OnSeek_003
 * @tc.desc  : Test OnSeek without strategy
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->OnSeek(3000);
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnSeek with no entries
 * @tc.number: OnSeek_004
 * @tc.desc  : Test OnSeek does nothing when entries_ is empty
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->OnSeek(5000);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test OnSeek resets played status for future ads
 * @tc.number: OnSeek_005
 * @tc.desc  : Test OnSeek resets played=false for ads with startMs >= seekTarget
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 5000, outId);
    controller_->entries_[outId].played = true;
    controller_->OnSeek(3000);
    EXPECT_FALSE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnSeek with seekTarget=0
 * @tc.number: OnSeek_006
 * @tc.desc  : Test OnSeek with seekTarget=0 marks no entries as played (startMs < 0 is impossible)
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_007, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->OnSeek(0);
    EXPECT_FALSE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnSeek(0) triggers pre-roll ad but DoSwitch fails
 * @tc.number: OnSeek_007
 * @tc.desc  : Test OnSeek(0) triggers pre-roll ad, DoSwitch fails, marks played and returns to IDLE
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_008, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");

    controller_->OnSeek(0);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->entries_[outId].played);
    EXPECT_TRUE(controller_->currentAdId_.empty());
}

/**
 * @tc.name  : Test OnSeek(0) with no pre-roll ad
 * @tc.number: OnSeek_008
 * @tc.desc  : Test OnSeek(0) with no pre-roll ad does not trigger DoSwitch
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_009, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/midroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 5000, outId);
    EXPECT_TRUE(controller_->preRollAdId_.empty());

    controller_->OnSeek(0);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_FALSE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnSeek(non-zero) does not trigger pre-roll ad
 * @tc.number: OnSeek_009
 * @tc.desc  : Test OnSeek(non-zero) does not trigger pre-roll ad
 */
HWTEST_F(InterstitialControllerUnitTest, OnSeek_010, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");

    controller_->OnSeek(5000);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->entries_[outId].played);
}

// ==================== OnPreloadTick ====================

/**
 * @tc.name  : Test OnPreloadTick
 * @tc.number: Tick_001
 * @tc.desc  : Test OnPreloadTick with ads disabled
 */
HWTEST_F(InterstitialControllerUnitTest, Tick_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->DisableAllAdsMediaSource();
    controller_->OnPreloadTick();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test OnPreloadTick
 * @tc.number: Tick_002
 * @tc.desc  : Test OnPreloadTick in PLAYING state
 */
HWTEST_F(InterstitialControllerUnitTest, Tick_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->OnPreloadTick();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::PLAYING);
}

/**
 * @tc.name  : Test OnPreloadTick
 * @tc.number: Tick_003
 * @tc.desc  : Test OnPreloadTick with null syncMgr
 */
HWTEST_F(InterstitialControllerUnitTest, Tick_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, nullptr);
    controller_->OnPreloadTick();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test OnPreloadTick with due entries triggering DoSwitch
 * @tc.number: Tick_004
 * @tc.desc  : Test OnPreloadTick finds due ad and calls DoSwitch which fails
 */
HWTEST_F(InterstitialControllerUnitTest, Tick_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(1500000));
    controller_->OnPreloadTick();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnPreloadTick with no due entries
 * @tc.number: Tick_005
 * @tc.desc  : Test OnPreloadTick does not call DoSwitch when no entries are due
 */
HWTEST_F(InterstitialControllerUnitTest, Tick_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 10000, outId);
    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(5000000));
    controller_->OnPreloadTick();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_FALSE(controller_->entries_[outId].played);
}

// ==================== OnAdEos ====================

/**
 * @tc.name  : Test OnAdEos
 * @tc.number: Eos_001
 * @tc.desc  : Test OnAdEos in IDLE state
 */
HWTEST_F(InterstitialControllerUnitTest, Eos_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->OnAdEos();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test OnAdEos in PLAYING state
 * @tc.number: Eos_002
 * @tc.desc  : Test OnAdEos emits END(COMPLETED) and calls DoResume
 */
HWTEST_F(InterstitialControllerUnitTest, Eos_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId;
    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });
    controller_->OnAdEos();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(receivedEvent.type, AdsEventType::COMPLETED);
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test OnAdEos with entry not found
 * @tc.number: Eos_003
 * @tc.desc  : Test OnAdEos handles case where currentAdId_ not in entries
 */
HWTEST_F(InterstitialControllerUnitTest, Eos_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = "non_existent";
    controller_->OnAdEos();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->currentAdId_.empty());
}

// ==================== TryPreRollAd ====================

/**
 * @tc.name  : Test TryPreRollAd with pre-roll ad recorded
 * @tc.number: TryPreRoll_001
 * @tc.desc  : Test TryPreRollAd triggers TrySwitchAdCandidates when preRollAdId_ is set
 */
HWTEST_F(InterstitialControllerUnitTest, TryPreRoll_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(controller_->preRollAdId_, "ad_0");

    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->entries_[outId].played);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test TryPreRollAd with no pre-roll ad
 * @tc.number: TryPreRoll_002
 * @tc.desc  : Test TryPreRollAd does nothing when preRollAdId_ is empty
 */
HWTEST_F(InterstitialControllerUnitTest, TryPreRoll_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    EXPECT_TRUE(controller_->preRollAdId_.empty());
    controller_->TryPreRollAd();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test TryPreRollAd with ads disabled
 * @tc.number: TryPreRoll_003
 * @tc.desc  : Test TryPreRollAd clears preRollAdId_ when ads are disabled
 */
HWTEST_F(InterstitialControllerUnitTest, TryPreRoll_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    controller_->DisableAllAdsMediaSource();

    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test TryPreRollAd when playState is not IDLE
 * @tc.number: TryPreRoll_004
 * @tc.desc  : Test TryPreRollAd clears preRollAdId_ when playState is PLAYING
 */
HWTEST_F(InterstitialControllerUnitTest, TryPreRoll_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(controller_->preRollAdId_, outId);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

/**
 * @tc.name  : Test TryPreRollAd when preRollAdId_ not found in entries
 * @tc.number: TryPreRoll_005
 * @tc.desc  : Test TryPreRollAd clears preRollAdId_ when entry was removed
 */
HWTEST_F(InterstitialControllerUnitTest, TryPreRoll_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->preRollAdId_ = "non_existent";
    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

/**
 * @tc.name  : Test TryPreRollAd when pre-roll ad already played
 * @tc.number: TryPreRoll_006
 * @tc.desc  : Test TryPreRollAd clears preRollAdId_ when entry is already played
 */
HWTEST_F(InterstitialControllerUnitTest, TryPreRoll_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    controller_->entries_[outId].played = true;
    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

// ==================== HasPendingEvents ====================

/**
 * @tc.name  : Test HasPendingEvents
 * @tc.number: Pending_001
 * @tc.desc  : Test HasPendingEvents with no entries
 */
HWTEST_F(InterstitialControllerUnitTest, Pending_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    EXPECT_FALSE(controller_->HasPendingEvents());
}

/**
 * @tc.name  : Test HasPendingEvents
 * @tc.number: Pending_002
 * @tc.desc  : Test HasPendingEvents with unplayed entry
 */
HWTEST_F(InterstitialControllerUnitTest, Pending_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    EXPECT_TRUE(controller_->HasPendingEvents());
}

/**
 * @tc.name  : Test HasPendingEvents
 * @tc.number: Pending_003
 * @tc.desc  : Test HasPendingEvents with all played entries
 */
HWTEST_F(InterstitialControllerUnitTest, Pending_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->entries_[outId].played = true;
    EXPECT_FALSE(controller_->HasPendingEvents());
}

// ==================== FindNextAdId ====================

/**
 * @tc.name  : Test FindNextAdId
 * @tc.number: FindNext_001
 * @tc.desc  : Test FindNextAdId marks ad expired when currentPosMs exceeds expiry window
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto nextId = controller_->FindNextAdId(5000);
    EXPECT_TRUE(nextId.empty());
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test FindNextAdId
 * @tc.number: FindNext_002
 * @tc.desc  : Test FindNextAdId with due ad
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto nextId = controller_->FindNextAdId(1100);
    EXPECT_EQ(nextId, outId);
}

/**
 * @tc.name  : Test FindNextAdId
 * @tc.number: FindNext_003
 * @tc.desc  : Test FindNextAdId with no ads
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto nextId = controller_->FindNextAdId(1500);
    EXPECT_TRUE(nextId.empty());
}

/**
 * @tc.name  : Test FindNextAdId
 * @tc.number: FindNext_004
 * @tc.desc  : Test FindNextAdId with ad not yet due
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 10000, outId);
    auto nextId = controller_->FindNextAdId(1500);
    EXPECT_TRUE(nextId.empty());
}

/**
 * @tc.name  : Test FindNextAdId
 * @tc.number: FindNext_005
 * @tc.desc  : Test FindNextAdId with multiple ads, select earliest
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source1, 5000, outId1);
    controller_->AddAdsMediaSource(source2, 1000, outId2);
    auto nextId = controller_->FindNextAdId(1100);
    EXPECT_EQ(nextId, outId2);
}

/**
 * @tc.name  : Test FindNextAdId
 * @tc.number: FindNext_006
 * @tc.desc  : Test FindNextAdId marks ad expired at boundary of expiry window (startMs + 200ms)
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto nextId = controller_->FindNextAdId(2000);
    EXPECT_TRUE(nextId.empty());
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test FindNextAdId with played entry
 * @tc.number: FindNext_007
 * @tc.desc  : Test FindNextAdId skips played entries
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_007, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source1, 1000, outId1);
    controller_->AddAdsMediaSource(source2, 2000, outId2);
    controller_->entries_[outId1].played = true;
    auto nextId = controller_->FindNextAdId(2100);
    EXPECT_EQ(nextId, outId2);
}

/**
 * @tc.name  : Test FindNextAdId with 2x speed extends expiry window
 * @tc.number: FindNext_008
 * @tc.desc  : Test FindNextAdId expiry window is extended by speed factor (2x: 200->400ms)
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_010, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    ON_CALL(*mockSyncMgr_, GetPlaybackRate()).WillByDefault(Return(2.0f));
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto nextId = controller_->FindNextAdId(1300);
    EXPECT_EQ(nextId, outId);
}

/**
 * @tc.name  : Test FindNextAdId with 2x speed expires correctly
 * @tc.number: FindNext_009
 * @tc.desc  : Test FindNextAdId marks ad expired when currentPosMs > startMs + 400ms at 2x speed
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_011, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    ON_CALL(*mockSyncMgr_, GetPlaybackRate()).WillByDefault(Return(2.0f));
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto nextId = controller_->FindNextAdId(1500);
    EXPECT_TRUE(nextId.empty());
    EXPECT_TRUE(controller_->entries_[outId].played);
}

/**
 * @tc.name  : Test FindNextAdId with syncMgr_ null
 * @tc.number: FindNext_010
 * @tc.desc  : Test FindNextAdId returns empty when syncMgr_ is null
 */
HWTEST_F(InterstitialControllerUnitTest, FindNext_012, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->syncMgr_ = nullptr;
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    auto nextId = controller_->FindNextAdId(1100);
    EXPECT_TRUE(nextId.empty());
}

// ==================== EmitStartEvent / EmitEndEvent ====================

/**
 * @tc.name  : Test EmitStartEvent
 * @tc.number: Emit_001
 * @tc.desc  : Test EmitStartEvent with callback
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });
    InterstitialEntry entry;
    entry.eventId = "test_id";
    entry.startMs = 1000;
    entry.durationMs = 5000;
    controller_->EmitStartEvent(entry);
    EXPECT_EQ(receivedEvent.type, AdsEventType::STARTED);
    EXPECT_EQ(receivedEvent.eventId, "test_id");
    EXPECT_EQ(receivedEvent.durationMs, 5000);
}

/**
 * @tc.name  : Test EmitStartEvent
 * @tc.number: Emit_002
 * @tc.desc  : Test EmitStartEvent without callback
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    InterstitialEntry entry;
    entry.eventId = "test_id";
    controller_->EmitStartEvent(entry);
}

/**
 * @tc.name  : Test EmitEndEvent
 * @tc.number: Emit_003
 * @tc.desc  : Test EmitEndEvent with callback
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });
    InterstitialEntry entry;
    entry.eventId = "test_id";
    entry.startMs = 1000;
    entry.durationMs = 5000;
    controller_->EmitEndEvent(entry, AdsEventType::COMPLETED);
    EXPECT_EQ(receivedEvent.type, AdsEventType::COMPLETED);
}

/**
 * @tc.name  : Test EmitEndEvent
 * @tc.number: Emit_004
 * @tc.desc  : Test EmitEndEvent with SKIPPED reason
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });
    InterstitialEntry entry;
    entry.eventId = "test_id";
    controller_->EmitEndEvent(entry, AdsEventType::SKIPPED);
    EXPECT_EQ(receivedEvent.type, AdsEventType::SKIPPED);
}

/**
 * @tc.name  : Test EmitEndEvent
 * @tc.number: Emit_005
 * @tc.desc  : Test EmitEndEvent with ERROR reason
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });
    InterstitialEntry entry;
    entry.eventId = "test_id";
    controller_->EmitEndEvent(entry, AdsEventType::ERROR, 5400103, "DoSourceSwitch failed");
    EXPECT_EQ(receivedEvent.type, AdsEventType::ERROR);
    EXPECT_EQ(receivedEvent.errorCode, 5400103);
    EXPECT_EQ(receivedEvent.errorMessage, "DoSourceSwitch failed");
}

/**
 * @tc.name  : Test EmitEndEvent without callback
 * @tc.number: Emit_006
 * @tc.desc  : Test EmitEndEvent does nothing when callback is null
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->adsEventCallback_ = nullptr;
    InterstitialEntry entry;
    entry.eventId = "test_id";
    controller_->EmitEndEvent(entry, AdsEventType::COMPLETED);
}

/**
 * @tc.name  : Test EmitStartEvent without callback
 * @tc.number: Emit_007
 * @tc.desc  : Test EmitStartEvent does nothing when callback is null
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_007, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->adsEventCallback_ = nullptr;
    InterstitialEntry entry;
    entry.eventId = "test_id";
    controller_->EmitStartEvent(entry);
}

/**
 * @tc.name  : Test EmitEndEvent with empty eventId skips callback
 * @tc.number: Emit_008
 * @tc.desc  : Test EmitEndEvent does not invoke callback when entry.eventId is empty
 */
HWTEST_F(InterstitialControllerUnitTest, Emit_008, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    bool callbackInvoked = false;
    controller_->SetAdsEventCallback([&callbackInvoked](const AdsChangeEvent& event) {
        callbackInvoked = true;
    });
    InterstitialEntry entry;
    entry.eventId = "";
    controller_->EmitEndEvent(entry, AdsEventType::COMPLETED);
    EXPECT_FALSE(callbackInvoked);
}

// ==================== DoSwitch ====================

/**
 * @tc.name  : Test DoSwitch with DoSourceSwitch failure rolls back
 * @tc.number: DoSwitch_001
 * @tc.desc  : Test DoSwitch rolls back to IDLE when DoSourceSwitch fails
 */
HWTEST_F(InterstitialControllerUnitTest, DoSwitch_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);

    EXPECT_FALSE(controller_->DoSwitch(outId));
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->currentAdId_.empty());
}

/**
 * @tc.name  : Test DoSwitch with already playing ad
 * @tc.number: DoSwitch_002
 * @tc.desc  : Test DoSwitch saves resume point only once (resumePointMs != 0 skips save)
 */
HWTEST_F(InterstitialControllerUnitTest, DoSwitch_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);

    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(8000000));

    controller_->resumePointMs_ = 3000;
    EXPECT_FALSE(controller_->DoSwitch(outId));
    EXPECT_EQ(controller_->resumePointMs_.load(), 3000);
}

/**
 * @tc.name  : Test DoSwitch with invalid entry id
 * @tc.number: DoSwitch_003
 * @tc.desc  : Test DoSwitch returns early when entry not found
 */
HWTEST_F(InterstitialControllerUnitTest, DoSwitch_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->DoSwitch("non_existent_id");
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test DoSwitch with VOD strategy saving resume point
 * @tc.number: DoSwitch_004
 * @tc.desc  : Test DoSwitch saves resumePointMs when using VOD strategy
 */
HWTEST_F(InterstitialControllerUnitTest, DoSwitch_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(5000000));
    controller_->resumePointMs_ = 0;
    controller_->DoSwitch(outId);
    EXPECT_EQ(controller_->resumePointMs_.load(), 5000);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test DoSwitch with Live strategy not saving resume point
 * @tc.number: DoSwitch_005
 * @tc.desc  : Test DoSwitch does not save resumePointMs when using Live strategy
 */
HWTEST_F(InterstitialControllerUnitTest, DoSwitch_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(true);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->resumePointMs_ = 0;
    controller_->DoSwitch(outId);
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

/**
 * @tc.name  : Test DoSwitch with syncMgr_ null
 * @tc.number: DoSwitch_006
 * @tc.desc  : Test DoSwitch does not crash when syncMgr_ is null
 */
HWTEST_F(InterstitialControllerUnitTest, DoSwitch_007, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, nullptr);
    controller_->SetLiveSource(false);
    controller_->syncMgr_ = nullptr;
    controller_->mainContentUri_ = "";
    mockDemuxer_->source_ = nullptr;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->entries_[outId].durationMs = 5000;
    controller_->DoSwitch(outId);
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

// ==================== DoResume ====================

/**
 * @tc.name  : Test DoResume with VOD strategy restores speed
 * @tc.number: DoResume_001
 * @tc.desc  : Test DoResume calls DoSourceSwitch with resumePointMs and restores speed
 */
HWTEST_F(InterstitialControllerUnitTest, DoResume_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->resumePointMs_ = 5000;
    controller_->originalSpeed_ = 1.5f;

    float receivedSpeed = 0.0f;
    controller_->SetSpeedChangeCallback([&receivedSpeed](float speed) {
        receivedSpeed = speed;
    });

    controller_->DoResume();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
    EXPECT_FLOAT_EQ(receivedSpeed, 1.5f);
}

/**
 * @tc.name  : Test DoResume with Live strategy
 * @tc.number: DoResume_002
 * @tc.desc  : Test DoResume uses Live strategy's GetResumeSeekMs (live edge)
 */
HWTEST_F(InterstitialControllerUnitTest, DoResume_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(true);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->resumePointMs_ = 5000;

    controller_->DoResume();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

/**
 * @tc.name  : Test DoResume with empty mainContentUri
 * @tc.number: DoResume_003
 * @tc.desc  : Test DoResume returns early when mainContentUri is empty
 */
HWTEST_F(InterstitialControllerUnitTest, DoResume_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->mainContentUri_ = "";
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->resumePointMs_ = 5000;
    controller_->DoResume();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

/**
 * @tc.name  : Test DoResume with null strategy and valid URI
 * @tc.number: DoResume_004
 * @tc.desc  : Test DoResume uses resumePointMs_ directly when strategy is null
 */
HWTEST_F(InterstitialControllerUnitTest, DoResume_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->scheduleStrategy_ = nullptr;
    controller_->SetMainContentUri("http://example.com/main.mp4");

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->resumePointMs_ = 5000;
    controller_->originalSpeed_ = 1.0f;

    controller_->DoResume();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

/**
 * @tc.name  : Test DoResume with Live strategy GetResumeSeekMs
 * @tc.number: DoResume_005
 * @tc.desc  : Test DoResume uses Live strategy's GetResumeSeekMs
 */
HWTEST_F(InterstitialControllerUnitTest, DoResume_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(true);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->resumePointMs_ = 5000;
    controller_->DoResume();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

/**
 * @tc.name  : Test DoResume with syncMgr_ null
 * @tc.number: DoResume_006
 * @tc.desc  : Test DoResume returns early when syncMgr_ is null
 */
HWTEST_F(InterstitialControllerUnitTest, DoResume_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, nullptr);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    controller_->syncMgr_ = nullptr;
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->resumePointMs_ = 5000;
    controller_->DoResume();
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(controller_->resumePointMs_.load(), 0);
}

// ==================== DoSourceSwitch ====================

/**
 * @tc.name  : Test DoSourceSwitch with SetDataSource failure
 * @tc.number: DoSourceSwitch_001
 * @tc.desc  : Test DoSourceSwitch returns error when SetDataSource fails
 */
HWTEST_F(InterstitialControllerUnitTest, DoSourceSwitch_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);

    auto ret = controller_->DoSourceSwitch("http://example.com/ad1.mp4", 0, 1.0f);
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name  : Test DoSourceSwitch with syncMgr null
 * @tc.number: DoSourceSwitch_002
 * @tc.desc  : Test DoSourceSwitch returns error when syncMgr_ is null
 */
HWTEST_F(InterstitialControllerUnitTest, DoSourceSwitch_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, nullptr);
    auto ret = controller_->DoSourceSwitch("http://example.com/ad1.mp4", 0, 1.0f);
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name  : Test DoSourceSwitch invokes speedChangeCallback
 * @tc.number: DoSourceSwitch_003
 * @tc.desc  : Test DoSourceSwitch calls speedChangeCallback_ with the given speed
 */
HWTEST_F(InterstitialControllerUnitTest, DoSourceSwitch_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);

    float receivedSpeed = 0.0f;
    controller_->SetSpeedChangeCallback([&receivedSpeed](float speed) {
        receivedSpeed = speed;
    });
    controller_->DoSourceSwitch("http://example.com/ad1.mp4", 0, 2.0f);
    EXPECT_FLOAT_EQ(receivedSpeed, 2.0f);
}

/**
 * @tc.name  : Test DoSourceSwitch with SetDataSource failure
 * @tc.number: DoSourceSwitch_004
 * @tc.desc  : Test DoSourceSwitch handles SetDataSource failure by returning error
 */
HWTEST_F(InterstitialControllerUnitTest, DoSourceSwitch_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    mockDemuxer_->source_ = nullptr;
    auto ret = controller_->DoSourceSwitch("http://example.com/ad1.mp4", 0, 1.0f);
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name  : Test DoSourceSwitch with preStartAction
 * @tc.number: DoSourceSwitch_005
 * @tc.desc  : Test DoSourceSwitch does not invoke preStartAction when SetDataSource fails
 */
HWTEST_F(InterstitialControllerUnitTest, DoSourceSwitch_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);

    bool preStartCalled = false;
    auto ret = controller_->DoSourceSwitch("http://example.com/ad1.mp4", 0, 1.0f, [&preStartCalled]() {
        preStartCalled = true;
    });
    EXPECT_NE(ret, Status::OK);
    EXPECT_FALSE(preStartCalled);
}

// ==================== ClampSeekMs ====================

/**
 * @tc.name  : Test ClampSeekMs when GetDuration returns false
 * @tc.number: ClampSeek_001
 * @tc.desc  : Test ClampSeekMs returns seekMs unchanged when duration is unavailable
 */
HWTEST_F(InterstitialControllerUnitTest, ClampSeek_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    EXPECT_EQ(controller_->ClampSeekMs(5000), 5000);
}

/**
 * @tc.name  : Test ClampSeekMs with seekMs=0
 * @tc.number: ClampSeek_002
 * @tc.desc  : Test ClampSeekMs returns 0 when seekMs is 0 and duration is unavailable
 */
HWTEST_F(InterstitialControllerUnitTest, ClampSeek_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    EXPECT_EQ(controller_->ClampSeekMs(0), 0);
}

// ==================== CollectSameStartMsAds ====================

/**
 * @tc.name  : Test CollectSameStartMsAds with no matching entries
 * @tc.number: Collect_001
 * @tc.desc  : Test CollectSameStartMsAds returns empty when no entries match startMs
 */
HWTEST_F(InterstitialControllerUnitTest, Collect_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 5000, outId);
    auto result = controller_->CollectSameStartMsAds(10000);
    EXPECT_TRUE(result.empty());
}

// ==================== SetSyncCenter / SetScheduler / SetMainContentUri ====================

/**
 * @tc.name  : Test SetSyncCenter
 * @tc.number: SetSyncCenter_001
 * @tc.desc  : Test SetSyncCenter
 */
HWTEST_F(InterstitialControllerUnitTest, SetSyncCenter_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, nullptr);
    controller_->SetSyncCenter(mockSyncMgr_);
    EXPECT_EQ(controller_->syncMgr_, mockSyncMgr_);
}

/**
 * @tc.name  : Test SetScheduler
 * @tc.number: SetScheduler_001
 * @tc.desc  : Test SetScheduler
 */
HWTEST_F(InterstitialControllerUnitTest, SetScheduler_001, TestSize.Level1)
{
    auto scheduler = std::make_shared<InterstitialScheduler>();
    controller_->SetScheduler(scheduler);
    EXPECT_EQ(controller_->scheduler_, scheduler);
}

/**
 * @tc.name  : Test SetMainContentUri
 * @tc.number: SetMainUri_001
 * @tc.desc  : Test SetMainContentUri
 */
HWTEST_F(InterstitialControllerUnitTest, SetMainUri_001, TestSize.Level1)
{
    controller_->SetMainContentUri("http://example.com/main.mp4");
    EXPECT_EQ(controller_->mainContentUri_, "http://example.com/main.mp4");
}

/**
 * @tc.name  : Test SetSpeedChangeCallback
 * @tc.number: SpeedCallback_001
 * @tc.desc  : Test SetSpeedChangeCallback sets callback correctly
 */
HWTEST_F(InterstitialControllerUnitTest, SpeedCallback_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    float receivedSpeed = 0.0f;
    controller_->SetSpeedChangeCallback([&receivedSpeed](float speed) {
        receivedSpeed = speed;
    });
    EXPECT_NE(controller_->speedChangeCallback_, nullptr);
}

// ==================== State queries ====================

/**
 * @tc.name  : Test IsPlayingInterstitial
 * @tc.number: IsPlaying_001
 * @tc.desc  : Test IsPlayingInterstitial
 */
HWTEST_F(InterstitialControllerUnitTest, IsPlaying_001, TestSize.Level1)
{
    EXPECT_FALSE(controller_->IsPlayingInterstitial());
    controller_->playState_ = InterstitialPlayState::PLAYING;
    EXPECT_TRUE(controller_->IsPlayingInterstitial());
}

/**
 * @tc.name  : Test IsAdsDisabled
 * @tc.number: IsDisabled_001
 * @tc.desc  : Test IsAdsDisabled
 */
HWTEST_F(InterstitialControllerUnitTest, IsDisabled_001, TestSize.Level1)
{
    EXPECT_FALSE(controller_->IsAdsDisabled());
    controller_->isDisableAds_ = true;
    EXPECT_TRUE(controller_->IsAdsDisabled());
}

/**
 * @tc.name  : Test GetPlayState
 * @tc.number: GetState_001
 * @tc.desc  : Test GetPlayState
 */
HWTEST_F(InterstitialControllerUnitTest, GetState_001, TestSize.Level1)
{
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    controller_->playState_ = InterstitialPlayState::PLAYING;
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::PLAYING);
}

/**
 * @tc.name  : Test SetPlayState
 * @tc.number: SetPlayState_001
 * @tc.desc  : Test SetPlayState changes state correctly
 */
HWTEST_F(InterstitialControllerUnitTest, SetPlayState_001, TestSize.Level1)
{
    controller_->SetPlayState(InterstitialPlayState::PLAYING);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::PLAYING);
    controller_->SetPlayState(InterstitialPlayState::IDLE);
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test GenerateAdId
 * @tc.number: GenId_001
 * @tc.desc  : Test GenerateAdId generates sequential ids
 */
HWTEST_F(InterstitialControllerUnitTest, GenId_001, TestSize.Level1)
{
    auto id1 = controller_->GenerateAdId();
    auto id2 = controller_->GenerateAdId();
    auto id3 = controller_->GenerateAdId();
    EXPECT_EQ(id1, "ad_0");
    EXPECT_EQ(id2, "ad_1");
    EXPECT_EQ(id3, "ad_2");
}

// ==================== Order preservation ====================

/**
 * @tc.name  : Test ads with same startMs are played in insertion order
 * @tc.number: Order_001
 * @tc.desc  : Test that ads with the same startMs but different URIs are played
 *             in the order they were added (ad_0 before ad_1 before ad_2)
 */
HWTEST_F(InterstitialControllerUnitTest, Order_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
    auto source3 = std::make_shared<Plugins::MediaSource>("http://example.com/ad3.mp4");

    std::string outId1, outId2, outId3;
    controller_->AddAdsMediaSource(source1, 5000, outId1);
    controller_->AddAdsMediaSource(source2, 5000, outId2);
    controller_->AddAdsMediaSource(source3, 5000, outId3);

    EXPECT_EQ(outId1, "ad_0");
    EXPECT_EQ(outId2, "ad_1");
    EXPECT_EQ(outId3, "ad_2");

    ON_CALL(*mockSyncMgr_, GetMediaTimeNow()).WillByDefault(Return(5000000));

    std::string nextId = controller_->FindNextAdId(5000);
    EXPECT_EQ(nextId, "ad_0");

    controller_->entries_["ad_0"].played = true;
    nextId = controller_->FindNextAdId(5000);
    EXPECT_EQ(nextId, "ad_1");

    controller_->entries_["ad_1"].played = true;
    nextId = controller_->FindNextAdId(5000);
    EXPECT_EQ(nextId, "ad_2");

    controller_->entries_["ad_2"].played = true;
    nextId = controller_->FindNextAdId(5000);
    EXPECT_TRUE(nextId.empty());
}

// ==================== Continuous playback ====================

/**
 * @tc.name  : Test OnAdEos with next ad at same startMs
 * @tc.number: Continuous_001
 * @tc.desc  : Test that OnAdEos marks current ad played and attempts to switch
 *             to next ad at same startMs. DoSwitch fails in mock, loop marks all
 *             cascades and eventually both ads are marked played, state is IDLE.
 */
HWTEST_F(InterstitialControllerUnitTest, Continuous_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");

    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source1, 5000, outId1);
    controller_->AddAdsMediaSource(source2, 5000, outId2);

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId1;

    controller_->OnAdEos();

    EXPECT_TRUE(controller_->entries_[outId1].played);
    EXPECT_TRUE(controller_->entries_[outId2].played);
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test OnAdEos with no next ad at same startMs
 * @tc.number: Continuous_002
 * @tc.desc  : Test that OnAdEos resumes when no next ad at same startMs
 */
HWTEST_F(InterstitialControllerUnitTest, Continuous_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId1;
    controller_->AddAdsMediaSource(source1, 5000, outId1);

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId1;

    controller_->OnAdEos();

    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_TRUE(controller_->entries_[outId1].played);
}

/**
 * @tc.name  : Test SkipCurrentAdsMediaSource skips current and attempts next at same startMs
 * @tc.number: Continuous_003
 * @tc.desc  : Test that skipping current ad marks it played and attempts next ad
 *             at same startMs. DoSwitch fails in mock, loop marks all as played:
 *             outId1 and outId2 both played, outId3 (different startMs) untouched, state IDLE.
 */
HWTEST_F(InterstitialControllerUnitTest, Continuous_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
    auto source3 = std::make_shared<Plugins::MediaSource>("http://example.com/ad3.mp4");

    std::string outId1, outId2, outId3;
    controller_->AddAdsMediaSource(source1, 5000, outId1);
    controller_->AddAdsMediaSource(source2, 5000, outId2);
    controller_->AddAdsMediaSource(source3, 10000, outId3);

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId1;

    controller_->SkipCurrentAdsMediaSource();

    EXPECT_TRUE(controller_->entries_[outId1].played);
    EXPECT_TRUE(controller_->entries_[outId2].played);
    EXPECT_FALSE(controller_->entries_[outId3].played);
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test ERROR with next ad at same startMs, DoSwitch fails
 * @tc.number: Continuous_004
 * @tc.desc  : Test FinishCurrentAdAndContinue(ERROR) marks current ad played,
 *             attempts next ad. DoSwitch fails, loop marks all same-startMs
 *             ads as played, then DoResume, state returns to IDLE.
 */
HWTEST_F(InterstitialControllerUnitTest, Continuous_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");

    std::string outId1, outId2;
    controller_->AddAdsMediaSource(source1, 5000, outId1);
    controller_->AddAdsMediaSource(source2, 5000, outId2);

    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = outId1;

    controller_->FinishCurrentAdAndContinue(AdsEventType::ERROR);

    EXPECT_TRUE(controller_->entries_[outId1].played);
    EXPECT_TRUE(controller_->entries_[outId2].played);
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

// ==================== Thread Safety (Race Condition Fixes) ====================

/**
 * @tc.name  : Test EmitAdStartEvent with entry removed before call
 * @tc.number: RaceSafe_001
 * @tc.desc  : Test EmitAdStartEvent returns early when entry is removed
 *             between setting currentAdId_ and invoking EmitAdStartEvent
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_001, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->currentAdId_ = "removed_ad";

    bool callbackInvoked = false;
    controller_->SetAdsEventCallback([&callbackInvoked](const AdsChangeEvent& event) {
        callbackInvoked = true;
    });

    controller_->EmitAdStartEvent();
    EXPECT_FALSE(callbackInvoked);
}

/**
 * @tc.name  : Test EmitAdStartEvent with empty currentAdId_
 * @tc.number: RaceSafe_002
 * @tc.desc  : Test EmitAdStartEvent returns early when currentAdId_ is empty
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_002, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->currentAdId_.clear();

    bool callbackInvoked = false;
    controller_->SetAdsEventCallback([&callbackInvoked](const AdsChangeEvent& event) {
        callbackInvoked = true;
    });

    controller_->EmitAdStartEvent();
    EXPECT_FALSE(callbackInvoked);
}

/**
 * @tc.name  : Test EmitAdStartEvent updates duration and emits event
 * @tc.number: RaceSafe_003
 * @tc.desc  : Test EmitAdStartEvent reads duration from demuxer,
 *             updates entries_, and emits STARTED event
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_003, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);

    controller_->currentAdId_ = outId;
    mockDemuxer_->source_ = nullptr;

    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });

    controller_->EmitAdStartEvent();
    EXPECT_EQ(receivedEvent.type, AdsEventType::STARTED);
    EXPECT_EQ(receivedEvent.eventId, outId);
}

/**
 * @tc.name  : Test DoSwitch clears currentAdId_ on failure under lock
 * @tc.number: RaceSafe_004
 * @tc.desc  : Test DoSwitch error path clears currentAdId_ safely
 *             and emits ERROR event with the pre-copied entry
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_004, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);

    AdsChangeEvent receivedEvent;
    controller_->SetAdsEventCallback([&receivedEvent](const AdsChangeEvent& event) {
        receivedEvent = event;
    });

    EXPECT_FALSE(controller_->DoSwitch(outId));
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
    EXPECT_EQ(receivedEvent.type, AdsEventType::ERROR);
    EXPECT_EQ(receivedEvent.eventId, outId);
}

/**
 * @tc.name  : Test DoResume clears currentAdId_ under lock
 * @tc.number: RaceSafe_005
 * @tc.desc  : Test DoResume error path (empty URI) clears currentAdId_ under lock
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_005, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->mainContentUri_ = "";
    controller_->syncMgr_ = nullptr;
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = "test_ad";
    controller_->resumePointMs_ = 5000;

    controller_->DoResume();
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test DoResume normal path clears currentAdId_ under lock
 * @tc.number: RaceSafe_006
 * @tc.desc  : Test DoResume normal path clears currentAdId_ under lock
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_006, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    controller_->playState_ = InterstitialPlayState::PLAYING;
    controller_->currentAdId_ = "test_ad";
    controller_->resumePointMs_ = 5000;
    controller_->originalSpeed_ = 1.0f;

    controller_->DoResume();
    EXPECT_TRUE(controller_->currentAdId_.empty());
    EXPECT_EQ(controller_->GetPlayState(), InterstitialPlayState::IDLE);
}

/**
 * @tc.name  : Test preRollAdId_ is set under lock in AddAdsMediaSource
 * @tc.number: RaceSafe_007
 * @tc.desc  : Test AddAdsMediaSource sets preRollAdId_ under adMutex_
 *             when startMs=0 and state is IDLE
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_007, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    auto ret = controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(controller_->preRollAdId_, outId);
}

/**
 * @tc.name  : Test preRollAdId_ not overwritten by second pre-roll
 * @tc.number: RaceSafe_008
 * @tc.desc  : Test AddAdsMediaSource does not overwrite preRollAdId_
 *             when it is already set (lock-protected check)
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_008, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source1 = std::make_shared<Plugins::MediaSource>("http://example.com/preroll1.mp4");
    auto source2 = std::make_shared<Plugins::MediaSource>("http://example.com/preroll2.mp4");
    std::string outId1, outId2;

    controller_->AddAdsMediaSource(source1, 0, outId1);
    EXPECT_EQ(controller_->preRollAdId_, outId1);

    controller_->AddAdsMediaSource(source2, 0, outId2);
    EXPECT_EQ(controller_->preRollAdId_, outId1);
}

/**
 * @tc.name  : Test TryPreRollAd accesses preRollAdId_ under lock
 * @tc.number: RaceSafe_009
 * @tc.desc  : Test TryPreRollAd clears preRollAdId_ under lock when disabled
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_009, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    EXPECT_FALSE(controller_->preRollAdId_.empty());

    controller_->DisableAllAdsMediaSource();
    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

/**
 * @tc.name  : Test TryPreRollAd clears preRollAdId_ when entry already played
 * @tc.number: RaceSafe_010
 * @tc.desc  : Test TryPreRollAd clears preRollAdId_ under lock
 *             when the entry has already been played
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_010, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 0, outId);
    controller_->entries_[outId].played = true;

    controller_->TryPreRollAd();
    EXPECT_TRUE(controller_->preRollAdId_.empty());
}

/**
 * @tc.name  : Test callback reentrancy does not deadlock in DoSwitch error
 * @tc.number: RaceSafe_011
 * @tc.desc  : Test DoSwitch error path: EmitEndEvent callback calls back
 *             into controller (e.g. AddAdsMediaSource) without deadlock
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_011, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);

    bool reentrantCalled = false;
    controller_->SetAdsEventCallback([&](const AdsChangeEvent& event) {
        if (event.type == AdsEventType::ERROR) {
            auto src2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
            std::string outId2;
            controller_->AddAdsMediaSource(src2, 2000, outId2);
            reentrantCalled = true;
        }
    });

    EXPECT_FALSE(controller_->DoSwitch(outId));
    EXPECT_TRUE(reentrantCalled);
}

/**
 * @tc.name  : Test callback reentrancy does not deadlock in EmitAdStartEvent
 * @tc.number: RaceSafe_012
 * @tc.desc  : Test EmitAdStartEvent: EmitStartEvent callback calls back
 *             into controller (e.g. AddAdsMediaSource) without deadlock
 */
HWTEST_F(InterstitialControllerUnitTest, RaceSafe_012, TestSize.Level1)
{
    controller_->Init(mockDemuxer_, mockSyncMgr_);
    controller_->SetLiveSource(false);
    controller_->SetMainContentUri("http://example.com/main.mp4");
    mockDemuxer_->source_ = nullptr;

    auto source = std::make_shared<Plugins::MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller_->AddAdsMediaSource(source, 1000, outId);
    controller_->currentAdId_ = outId;

    bool reentrantCalled = false;
    controller_->SetAdsEventCallback([&](const AdsChangeEvent& event) {
        if (event.type == AdsEventType::STARTED) {
            auto src2 = std::make_shared<Plugins::MediaSource>("http://example.com/ad2.mp4");
            std::string outId2;
            controller_->AddAdsMediaSource(src2, 2000, outId2);
            reentrantCalled = true;
        }
    });

    controller_->EmitAdStartEvent();
    EXPECT_TRUE(reentrantCalled);
}

} // namespace Media
} // namespace OHOS