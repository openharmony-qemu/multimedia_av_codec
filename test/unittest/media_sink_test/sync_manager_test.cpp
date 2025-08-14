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
#include <queue>
#include "gtest/gtest.h"
#include "filter/filter.h"
#include "media_sync_manager.h"

#include "video_sink.h"
#include "media_synchronous_sink.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {
using namespace Pipeline;
class TestSyncManager : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void) { }
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void) { }
    // SetUp: Called before each test cases
    void SetUp(void)
    {
        syncManager_ = std::make_shared<MediaSyncManager>();
        ASSERT_TRUE(syncManager_ != nullptr);
    }
    // TearDown: Called after each test cases
    void TearDown(void)
    {
        syncManager_ = nullptr;
    }
public:
    std::shared_ptr<MediaSyncManager> syncManager_ = nullptr;
};

HWTEST_F(TestSyncManager, sync_manager_set, TestSize.Level1)
{
    float rate = 0;
    auto setPlaybackRateStatus = syncManager_->SetPlaybackRate(rate);
    ASSERT_EQ(setPlaybackRateStatus, Status::OK);
}

HWTEST_F(TestSyncManager, sync_manager_get, TestSize.Level1)
{
    auto rateBack = syncManager_->GetPlaybackRate();
    ASSERT_EQ(rateBack, 1);
}

HWTEST_F(TestSyncManager, sync_manager_life_cycle, TestSize.Level1)
{
    // Resume
    auto resumeStatus = syncManager_->Resume();
    ASSERT_EQ(resumeStatus, Status::OK);

    // Pause
    auto pauseStatus = syncManager_->Pause();
    ASSERT_EQ(pauseStatus, Status::OK);

    // Seek
    int64_t seekTime = 0;
    auto seekStatus = syncManager_->Seek(seekTime);
    ASSERT_NE(seekStatus, Status::OK);

    // Reset
    auto resetStatus = syncManager_->Reset();
    ASSERT_EQ(resetStatus, Status::OK);

    // IsSeeking
    auto isSeeking = syncManager_->InSeeking();
    ASSERT_EQ(isSeeking, false);
}

HWTEST_F(TestSyncManager, sync_manager_update_time_without_synchronizer, TestSize.Level1)
{
    // AddSynchronizer
    VideoSink sync;
    syncManager_->AddSynchronizer(&sync);
    // RemoveSynchronizer
    syncManager_->RemoveSynchronizer(&sync);
    // UpdateTimeAnchor
    auto updateTimeStatus = syncManager_->UpdateTimeAnchor(-1, -1, {-1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, -1, {-1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {-1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, 1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, 1, 1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, 1, 1}, &sync);
    ASSERT_EQ(updateTimeStatus, true);
}

HWTEST_F(TestSyncManager, sync_manager_life_func, TestSize.Level1)
{
    // AddSynchronizer
    VideoSink sync;
    syncManager_->AddSynchronizer(&sync);
    // UpdateTimeAnchor
    auto updateTimeStatus = syncManager_->UpdateTimeAnchor(-1, -1, {-1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, -1, {-1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {-1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, -1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, 1, -1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, 1, 1}, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, {1, 1, 1}, &sync);
    ASSERT_EQ(updateTimeStatus, true);

    // GetMediaTimeNow
    auto mediaTimeNow = syncManager_->GetMediaTimeNow();
    ASSERT_EQ(mediaTimeNow, 0);

    // GetClockTimeNow
    auto clockTimeNow = syncManager_->GetMediaTimeNow();
    ASSERT_EQ(clockTimeNow, 0);

    // GetAnchoredClockTime
    auto clockTime = syncManager_->GetAnchoredClockTime(0);
    ASSERT_NE(clockTime, 0);

    // Report
    syncManager_->ReportPrerolled(&sync);
    syncManager_->ReportEos(&sync);
    syncManager_->SetMediaTimeRangeEnd(0, 0, &sync);
    syncManager_->SetMediaTimeRangeStart(0, 0, &sync);

    // GetSeekTime
    int64_t seekTime = syncManager_->GetSeekTime();
    ASSERT_NE(seekTime, 0);
}

// Scenario1: Test when alreadySetSyncersShouldWait_ is false.
HWTEST_F(TestSyncManager, SetAllSyncShouldWaitNoLock_001, TestSize.Level0)
{
    syncManager_->alreadySetSyncersShouldWait_ = false;
    syncManager_->SetAllSyncShouldWaitNoLock();
    EXPECT_TRUE(syncManager_->prerolledSyncers_.empty());
    EXPECT_TRUE(syncManager_->alreadySetSyncersShouldWait_);
}

// Scenario3: Test when alreadySetSyncersShouldWait_ is true.
HWTEST_F(TestSyncManager, SetAllSyncShouldWaitNoLock_003, TestSize.Level0)
{
    syncManager_->alreadySetSyncersShouldWait_ = true;
    syncManager_->SetAllSyncShouldWaitNoLock();
    EXPECT_TRUE(syncManager_->prerolledSyncers_.empty());
    EXPECT_TRUE(syncManager_->alreadySetSyncersShouldWait_);
}

// Scenario1: Test the normal flow of the Resume method
HWTEST_F(TestSyncManager, Resume_001, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::RESUMED;
    EXPECT_EQ(syncManager_->Resume(), Status::OK);
}

HWTEST_F(TestSyncManager, Resume_002, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedMediaTime_ = 100;
    syncManager_->alreadySetSyncersShouldWait_ = true;
    EXPECT_EQ(syncManager_->Resume(), Status::OK);
    EXPECT_EQ(syncManager_->pausedMediaTime_, HST_TIME_NONE);
    EXPECT_EQ(syncManager_->pausedClockTime_, HST_TIME_NONE);
}

// Scenario3: Test the flow of the Resume method when clockState is not paused and not resumed
HWTEST_F(TestSyncManager, Resume_003, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    EXPECT_EQ(syncManager_->Resume(), Status::OK);
    EXPECT_EQ(syncManager_->clockState_, MediaSyncManager::State::RESUMED);
}

HWTEST_F(TestSyncManager, Pause_001, TestSize.Level0)
{
    EXPECT_EQ(syncManager_->Pause(), Status::OK);
}

HWTEST_F(TestSyncManager, Pause_002, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::RESUMED;
    syncManager_->currentAnchorMediaTime_ = 100;
    syncManager_->currentAnchorClockTime_ = 100;
    EXPECT_EQ(syncManager_->Pause(), Status::OK);
}

// Scenario1: Seek when minRangeStartOfMediaTime_ is HST_TIME_NONE
HWTEST_F(TestSyncManager, Seek_ShouldReturnErrorInvalidOperation_WhenMinRangeStartIsNone, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = HST_TIME_NONE;
    syncManager_->maxRangeEndOfMediaTime_ = 100;
    EXPECT_EQ(syncManager_->Seek(50, true), Status::ERROR_INVALID_OPERATION);
}

// Scenario2: Seek when maxRangeEndOfMediaTime_ is HST_TIME_NONE
HWTEST_F(TestSyncManager, Seek_ShouldReturnErrorInvalidOperation_WhenMaxRangeEndIsNone, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 0;
    syncManager_->maxRangeEndOfMediaTime_ = HST_TIME_NONE;
    EXPECT_EQ(syncManager_->Seek(50, true), Status::ERROR_INVALID_OPERATION);
}

// Scenario3: Seek when isClosest is true
HWTEST_F(TestSyncManager, Seek_ShouldSetFirstMediaTimeAfterSeek_WhenIsClosestIsTrue, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 0;
    syncManager_->maxRangeEndOfMediaTime_ = 100;
    EXPECT_EQ(syncManager_->Seek(50, true), Status::OK);
    EXPECT_EQ(syncManager_->firstMediaTimeAfterSeek_, 50);
}

// Scenario4: Seek when isClosest is false
HWTEST_F(TestSyncManager, Seek_ShouldSetFirstMediaTimeAfterSeekToNone_WhenIsClosestIsFalse, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 0;
    syncManager_->maxRangeEndOfMediaTime_ = 100;
    EXPECT_EQ(syncManager_->Seek(50, false), Status::OK);
    EXPECT_EQ(syncManager_->firstMediaTimeAfterSeek_, HST_TIME_NONE);
}

// Scenario1: Test when syncer is nullptr then AddSynchronizer does nothing.
HWTEST_F(TestSyncManager, AddSynchronizer_ShouldDoNothing_WhenSyncerIsNull, TestSize.Level0)
{
    IMediaSynchronizer* syncer = nullptr;
    syncManager_->AddSynchronizer(syncer);
    // No exception should be thrown and no action should be performed.
    EXPECT_EQ(syncManager_->syncers_.size(), 0);
}

// Scenario2: Test when syncer is not nullptr and not already in syncers_ then AddSynchronizer adds the syncer.
HWTEST_F(TestSyncManager, AddSynchronizer_ShouldAddSyncer_WhenSyncerIsNotNullAndNotInSyncers, TestSize.Level0)
{
    IMediaSynchronizer* syncer = new VideoSink();
    syncManager_->AddSynchronizer(syncer);
    EXPECT_EQ(syncManager_->syncers_.size(), 1);
    EXPECT_EQ(syncManager_->syncers_[0], syncer);
    delete syncer;
}

// Scenario3: Test when syncer is not nullptr and already in syncers_ then AddSynchronizer does nothing.
HWTEST_F(TestSyncManager, AddSynchronizer_ShouldDoNothing_WhenSyncerIsNotNullAndInSyncers, TestSize.Level0)
{
    IMediaSynchronizer* syncer = new VideoSink();
    syncManager_->AddSynchronizer(syncer);
    syncManager_->AddSynchronizer(syncer);
    EXPECT_EQ(syncManager_->syncers_.size(), 1);
    delete syncer;
}

// Scenario2: Test when syncer is not nullptr then RemoveSynchronizer removes the syncer from syncManager.
HWTEST_F(TestSyncManager, RemoveSynchronizer_ShouldRemoveSyncer_WhenSyncerIsNotNull, TestSize.Level0)
{
    IMediaSynchronizer* syncer = new VideoSink();
    IMediaSynchronizer* syncerOne = new VideoSink();
    syncManager_->AddSynchronizer(syncer);
    syncManager_->RemoveSynchronizer(syncerOne);
    EXPECT_EQ(syncManager_->syncers_.size(), 1);
    syncManager_->RemoveSynchronizer(syncer);
    EXPECT_EQ(syncManager_->syncers_.size(), 0);
    delete syncer;
    delete syncerOne;
}

// Scenario1: Test case for rate less than 0
HWTEST_F(TestSyncManager, SetPlaybackRate_ShouldReturnError_WhenRateLessThan0, TestSize.Level0) {
    float rate = -1.0;
    EXPECT_EQ(syncManager_->SetPlaybackRate(rate), Status::ERROR_INVALID_PARAMETER);
}

// Scenario2: Test case for rate equal to 0
HWTEST_F(TestSyncManager, SetPlaybackRate_ShouldReturnOk_WhenRateEqualTo0, TestSize.Level0) {
    float rate = 0.0;
    EXPECT_EQ(syncManager_->SetPlaybackRate(rate), Status::OK);
}

// Scenario3: Test case for rate greater than 0 and currentAnchorMediaTime_ is HST_TIME_NONE
HWTEST_F(TestSyncManager,
    SetPlaybackRate_ShouldReturnOk_WhenRateGreaterThan0AndCurrentAnchorMediaTimeIsNone, TestSize.Level0) {
    float rate = 1.0;
    syncManager_->currentAnchorMediaTime_ = HST_TIME_NONE;
    EXPECT_EQ(syncManager_->SetPlaybackRate(rate), Status::OK);
}

// Scenario4: Test case for rate greater than 0 and currentAnchorMediaTime_ is not HST_TIME_NONE
HWTEST_F(TestSyncManager,
    SetPlaybackRate_ShouldReturnOk_WhenRateGreaterThan0AndCurrentAnchorMediaTimeIsNotNone, TestSize.Level0) {
    float rate = 1.0;
    syncManager_->currentAnchorMediaTime_ = 100;
    EXPECT_EQ(syncManager_->SetPlaybackRate(rate), Status::OK);
}

HWTEST_F(TestSyncManager, UpdateFirstPtsAfterSeek_ShouldUpdateFirstPts_WhenFirstPtsIsNone, TestSize.Level0)
{
    int64_t mediaTime = 100;
    syncManager_->UpdateFirstPtsAfterSeek(mediaTime);
    EXPECT_EQ(syncManager_->firstMediaTimeAfterSeek_, mediaTime);
}

HWTEST_F(TestSyncManager, UpdateFirstPtsAfterSeek_ShouldUpdateFirstPts_WhenMediaTimeIsGreater, TestSize.Level0)
{
    int64_t firstMediaTimeAfterSeek = 50;
    int64_t mediaTime = 100;
    syncManager_->firstMediaTimeAfterSeek_ = firstMediaTimeAfterSeek;
    syncManager_->UpdateFirstPtsAfterSeek(mediaTime);
    EXPECT_EQ(syncManager_->firstMediaTimeAfterSeek_, mediaTime);
}

HWTEST_F(TestSyncManager, UpdateFirstPtsAfterSeek_ShouldNotUpdateFirstPts_WhenMediaTimeIsLess, TestSize.Level0)
{
    int64_t firstMediaTimeAfterSeek = 150;
    int64_t mediaTime = 100;
    syncManager_->firstMediaTimeAfterSeek_ = firstMediaTimeAfterSeek;
    syncManager_->UpdateFirstPtsAfterSeek(mediaTime);
    EXPECT_EQ(syncManager_->firstMediaTimeAfterSeek_, firstMediaTimeAfterSeek);
}

HWTEST_F(TestSyncManager, GetMediaTime_001, TestSize.Level0)
{
    syncManager_->currentAnchorClockTime_ = 100;
    syncManager_->delayTime_ = 50;
    syncManager_->currentAnchorMediaTime_ = 200;
    syncManager_->playRate_ = 0;
    int64_t clockTime = 150;
    int64_t result = syncManager_->GetMediaTime(clockTime);
    ASSERT_EQ(result, HST_TIME_NONE);
}

HWTEST_F(TestSyncManager, GetMediaTime_002, TestSize.Level0)
{
    syncManager_->currentAnchorClockTime_ = HST_TIME_NONE;
    syncManager_->delayTime_ = 50;
    syncManager_->currentAnchorMediaTime_ = 200;
    syncManager_->playRate_ = 1.0;
    int64_t clockTime = 150;
    int64_t result = syncManager_->GetMediaTime(clockTime);
    ASSERT_EQ(result, HST_TIME_NONE);
}

HWTEST_F(TestSyncManager, GetMediaTime_003, TestSize.Level0)
{
    syncManager_->currentAnchorClockTime_ = 100;
    syncManager_->delayTime_ = 50;
    syncManager_->currentAnchorMediaTime_ = 200;
    syncManager_->playRate_ = 1.0;
    int64_t clockTime = 150;
    int64_t result = syncManager_->GetMediaTime(clockTime);
    ASSERT_EQ(result, 250);
}

HWTEST_F(TestSyncManager, GetAnchoredClockTime_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.minRangeStartOfMediaTime_ = 100;
    mediaSyncManager.maxRangeEndOfMediaTime_ = 200;
    int64_t mediaTime = 50;
    int64_t result = mediaSyncManager.GetAnchoredClockTime(mediaTime);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario2: Test when mediaTime is greater than maxRangeEndOfMediaTime_
HWTEST_F(TestSyncManager, GetAnchoredClockTime_002, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.minRangeStartOfMediaTime_ = 100;
    mediaSyncManager.maxRangeEndOfMediaTime_ = 200;
    int64_t mediaTime = 250;
    int64_t result = mediaSyncManager.GetAnchoredClockTime(mediaTime);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario3: Test when mediaTime is within the range
HWTEST_F(TestSyncManager, GetAnchoredClockTime_003, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.minRangeStartOfMediaTime_ = 100;
    mediaSyncManager.maxRangeEndOfMediaTime_ = 200;
    int64_t mediaTime = 150;
    int64_t result = mediaSyncManager.GetAnchoredClockTime(mediaTime);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario1: audio priority and not seeked, do not go back
HWTEST_F(TestSyncManager, BoundMediaProgress_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t newMediaProgressTime;
    int64_t lastReportMediaTime = 90;
    // pre-defined values
    mediaSyncManager.lastAudioBufferDuration_ = 20;
    mediaSyncManager.currentAnchorMediaTime_ = 100;
    mediaSyncManager.lastReportMediaTime_ = lastReportMediaTime;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::AUDIO_SINK;

    newMediaProgressTime = 50;
    auto result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, lastReportMediaTime);
}

// Scenario2: audio priority and not seeked, do not go too far
HWTEST_F(TestSyncManager, BoundMediaProgress_002, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t newMediaProgressTime;
    // pre-defined values
    mediaSyncManager.lastAudioBufferDuration_ = 20;
    mediaSyncManager.currentAnchorMediaTime_ = 100;
    mediaSyncManager.lastReportMediaTime_ = 90;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::AUDIO_SINK;

    newMediaProgressTime = 250;
    auto result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, mediaSyncManager.currentAnchorMediaTime_ + mediaSyncManager.lastAudioBufferDuration_);
}

// Scenario3: audio priority and not seeked, progress is valid
HWTEST_F(TestSyncManager, BoundMediaProgress_003, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t newMediaProgressTime;
    // pre-defined values
    mediaSyncManager.lastAudioBufferDuration_ = 20;
    mediaSyncManager.currentAnchorMediaTime_ = 100;
    mediaSyncManager.lastReportMediaTime_ = 90;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::AUDIO_SINK;

    newMediaProgressTime = 100;
    auto result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, newMediaProgressTime);
}

// Scenario4: non-audio priority and seeked, go back after seek
HWTEST_F(TestSyncManager, BoundMediaProgress_004, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t newMediaProgressTime;
    // pre-defined values
    mediaSyncManager.lastAudioBufferDuration_ = 100;
    mediaSyncManager.currentAnchorMediaTime_ = 100;
    mediaSyncManager.lastReportMediaTime_ = 150;
    mediaSyncManager.isFrameAfterSeeked_ = true;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;

    newMediaProgressTime = 50;
    auto result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, newMediaProgressTime);
}

// Scenario5: non-audio priority and seeked, go forward after seek
HWTEST_F(TestSyncManager, BoundMediaProgress_005, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t newMediaProgressTime;
    // pre-defined values
    mediaSyncManager.currentAnchorMediaTime_ = 200;
    mediaSyncManager.lastReportMediaTime_ = 150;
    mediaSyncManager.isFrameAfterSeeked_ = true;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;

    newMediaProgressTime = 175;
    auto result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, newMediaProgressTime);
}

// Scenario6: video audio priority and seeked, go forward after seek
HWTEST_F(TestSyncManager, BoundMediaProgress_006, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t newMediaProgressTime;
    // pre-defined values
    mediaSyncManager.lastVideoBufferPts_ = 0;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    newMediaProgressTime = 175;
    auto result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, mediaSyncManager.lastVideoBufferPts_);
}

// Scenario1: Test case when isSeeking_ is true
HWTEST_F(TestSyncManager, GetMediaTimeNow_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t seekingMediaTime = 100;
    mediaSyncManager.isSeeking_ = true;
    mediaSyncManager.seekingMediaTime_ = seekingMediaTime;
    EXPECT_EQ(mediaSyncManager.GetMediaTimeNow(), seekingMediaTime);
}

// Scenario2: Test case when paused
HWTEST_F(TestSyncManager, GetMediaTimeNow_002, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.audioRenderPts_ = 100;
    mediaSyncManager.isSeeking_ = false;
    mediaSyncManager.lastReportMediaTime_ = 100;
    mediaSyncManager.pausedMediaTime_ = 120;
    mediaSyncManager.currentAnchorMediaTime_ = 150;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.firstMediaTimeAfterSeek_ = 50;
    mediaSyncManager.clockState_ = MediaSyncManager::State::PAUSED;
    mediaSyncManager.startPts_ = 0;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
    int64_t result = mediaSyncManager.GetMediaTimeNow();
    EXPECT_EQ(result, mediaSyncManager.pausedMediaTime_);
}

// Scenario3: Test case when invalid
HWTEST_F(TestSyncManager, GetMediaTimeNow_003, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.isSeeking_ = false;
    mediaSyncManager.clockState_ = MediaSyncManager::State::RESUMED;
    mediaSyncManager.startPts_ = 0;
    mediaSyncManager.currentAnchorClockTime_ = HST_TIME_NONE;
    int64_t result = mediaSyncManager.GetMediaTimeNow();
    EXPECT_EQ(result, 0);
}

// Scenario4: Test for normal case1
HWTEST_F(TestSyncManager, GetMediaTimeNow_004, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t lastReportMediaTime_ = 110;
    mediaSyncManager.audioRenderPts_ = 100;
    mediaSyncManager.isSeeking_ = false;
    mediaSyncManager.clockState_ = MediaSyncManager::State::RESUMED;
    mediaSyncManager.lastReportMediaTime_ = lastReportMediaTime_;
    mediaSyncManager.currentAnchorMediaTime_ = 150;
    mediaSyncManager.delayTime_ = 50;
    mediaSyncManager.playRate_ = 1.0f;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.firstMediaTimeAfterSeek_ = 50; // firstMediaTimeAfterSeek_ < currentMediaTime
    mediaSyncManager.startPts_ = 0;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
    mediaSyncManager.currentAnchorClockTime_ = mediaSyncManager.GetSystemClock();
    int64_t result = mediaSyncManager.GetMediaTimeNow();
    EXPECT_EQ(result >= lastReportMediaTime_, true);
    EXPECT_EQ(result <= mediaSyncManager.currentAnchorMediaTime_, true);
}

// Scenario5: Test for normal case2
HWTEST_F(TestSyncManager, GetMediaTimeNow_005, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t lastReportMediaTime_ = 110;
    mediaSyncManager.audioRenderPts_ = 100;
    mediaSyncManager.isSeeking_ = false;
    mediaSyncManager.clockState_ = MediaSyncManager::State::RESUMED;
    mediaSyncManager.lastReportMediaTime_ = lastReportMediaTime_;
    mediaSyncManager.currentAnchorMediaTime_ = 150;
    mediaSyncManager.delayTime_ = 50;
    mediaSyncManager.playRate_ = 1.0f;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.firstMediaTimeAfterSeek_ = 150; // firstMediaTimeAfterSeek_ >= currentMediaTime
    mediaSyncManager.startPts_ = 0;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
    mediaSyncManager.currentAnchorClockTime_ = mediaSyncManager.GetSystemClock();
    int64_t result = mediaSyncManager.GetMediaTimeNow();
    EXPECT_EQ(result >= lastReportMediaTime_, true);
    EXPECT_EQ(result <= mediaSyncManager.currentAnchorMediaTime_, true);
}

// Scenario6: Test for normal case3
HWTEST_F(TestSyncManager, GetMediaTimeNow_006, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t lastReportMediaTime_ = 110;
    mediaSyncManager.audioRenderPts_ = 100;
    mediaSyncManager.isSeeking_ = false;
    mediaSyncManager.clockState_ = MediaSyncManager::State::RESUMED;
    mediaSyncManager.lastReportMediaTime_ = lastReportMediaTime_;
    mediaSyncManager.currentAnchorMediaTime_ = 150;
    mediaSyncManager.delayTime_ = 50;
    mediaSyncManager.playRate_ = 1.0f;
    mediaSyncManager.isFrameAfterSeeked_ = false;
    mediaSyncManager.firstMediaTimeAfterSeek_ = HST_TIME_NONE; // firstMediaTimeAfterSeek_ = HST_TIME_NONE
    mediaSyncManager.startPts_ = 0;
    mediaSyncManager.currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
    mediaSyncManager.currentAnchorClockTime_ = mediaSyncManager.GetSystemClock();
    int64_t result = mediaSyncManager.GetMediaTimeNow();
    EXPECT_EQ(result >= lastReportMediaTime_, true);
    EXPECT_EQ(result <= mediaSyncManager.currentAnchorMediaTime_, true);
}

HWTEST_F(TestSyncManager, GetMediaTimeNow_007, TestSize.Level0)
{
    syncManager_->audioRenderPts_ = 100;
    syncManager_->isSeeking_ = false;
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedMediaTime_ = 50;
    syncManager_->firstMediaTimeAfterSeek_ = 150;
    syncManager_->startPts_ = 50;
    syncManager_->currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
    syncManager_->currentAnchorMediaTime_ = 50;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 50);
}

HWTEST_F(TestSyncManager, GetMediaTimeNow_008, TestSize.Level0)
{
    syncManager_->audioRenderPts_ = 100;
    syncManager_->isSeeking_ = false;
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedMediaTime_ = 100;
    syncManager_->firstMediaTimeAfterSeek_ = 150;
    syncManager_->currentSyncerPriority_ = IMediaSynchronizer::SUBTITLE_SINK;
    syncManager_->currentAnchorMediaTime_ = 50;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 50);
}

HWTEST_F(TestSyncManager, GetMediaTimeNow_009, TestSize.Level0)
{
    syncManager_->isSeeking_ = false;
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedMediaTime_ = 100;
    syncManager_->firstMediaTimeAfterSeek_ = 150;
    syncManager_->currentAnchorMediaTime_ = 50;
    syncManager_->audioRenderPts_ = HST_TIME_NONE;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), HST_TIME_NONE);
}

HWTEST_F(TestSyncManager, GetMediaTimeNow_010, TestSize.Level0)
{
    syncManager_->isSeeking_ = false;
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedMediaTime_ = 100;
    syncManager_->firstMediaTimeAfterSeek_ = 150;
    syncManager_->currentAnchorMediaTime_ = 50;
    syncManager_->audioRenderPts_ = 0;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), HST_TIME_NONE);
}

HWTEST_F(TestSyncManager, SetLastAudioBufferDuration_001, TestSize.Level0)
{
    syncManager_->SetLastAudioBufferDuration(50);
    EXPECT_EQ(syncManager_->lastAudioBufferDuration_, 50);
    syncManager_->SetLastAudioBufferDuration(0);
    EXPECT_EQ(syncManager_->lastAudioBufferDuration_, 0);
}

// Scenario1: Test when startPts is less than startPts_ then startPts_ should be updated.
HWTEST_F(TestSyncManager, SetMediaStartPts_ShouldUpdateStartPts_WhenStartPtsIsLess, TestSize.Level0)
{
    syncManager_->SetMediaStartPts(100);
    syncManager_->SetMediaStartPts(50);
    EXPECT_EQ(syncManager_->startPts_, 50);
}

// Scenario2: Test when startPts is equal to startPts_ then startPts_ should not be updated.
HWTEST_F(TestSyncManager, SetMediaStartPts_ShouldNotUpdateStartPts_WhenStartPtsIsEqual, TestSize.Level0)
{
    syncManager_->SetMediaStartPts(100);
    syncManager_->SetMediaStartPts(100);
    EXPECT_EQ(syncManager_->startPts_, 100);
}

// Scenario3: Test when startPts is HST_TIME_NONE then startPts_ should be updated.
HWTEST_F(TestSyncManager, SetMediaStartPts_ShouldUpdateStartPts_WhenStartPtsIsNone, TestSize.Level0)
{
    syncManager_->SetMediaStartPts(HST_TIME_NONE);
    EXPECT_EQ(syncManager_->startPts_, HST_TIME_NONE);
}

// Scenario1: Test when supplier is nullptr then ReportPrerolled returns immediately.
HWTEST_F(TestSyncManager, ReportPrerolled_001, TestSize.Level0)
{
    syncManager_->ReportPrerolled(nullptr);
    // No further action is expected, as the function should return immediately.
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 0);
}

// Scenario3: Test when supplier is already in prerolledSyncers_ then ReportPrerolled returns immediately.
HWTEST_F(TestSyncManager, ReportPrerolled_003, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->prerolledSyncers_.emplace_back(supplier);
    syncManager_->ReportPrerolled(supplier);
    // No further action is expected, as the function should return immediately.
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 1);
    delete supplier;
}

HWTEST_F(TestSyncManager, ReportPrerolled_004, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->ReportPrerolled(supplier);
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 1);
    EXPECT_EQ(syncManager_->prerolledSyncers_.front(), supplier);
    delete supplier;
}

// Scenario5: Test when all prerolledSyncers_ are equal to syncers_ then all prerolledSyncers_ are notified and cleared.
HWTEST_F(TestSyncManager, ReportPrerolled_005, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->syncers_.emplace_back(supplier);
    syncManager_->ReportPrerolled(supplier);
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 0);
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 0);
    delete supplier;
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS