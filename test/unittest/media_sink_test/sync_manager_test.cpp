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
    auto updateTimeStatus = syncManager_->UpdateTimeAnchor(-1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, 1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, 1, 1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, 1, 1, &sync);
    ASSERT_EQ(updateTimeStatus, true);
}

HWTEST_F(TestSyncManager, sync_manager_life_func, TestSize.Level1)
{
    // AddSynchronizer
    VideoSink sync;
    syncManager_->AddSynchronizer(&sync);
    // UpdateTimeAnchor
    auto updateTimeStatus = syncManager_->UpdateTimeAnchor(-1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, -1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, -1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, -1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, 1, -1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, 1, 1, nullptr);
    ASSERT_EQ(updateTimeStatus, true);
    updateTimeStatus = syncManager_->UpdateTimeAnchor(1, 1, 1, 1, 1, &sync);
    ASSERT_EQ(updateTimeStatus, true);

    // GetMediaTimeNow
    auto mediaTimeNow = syncManager_->GetMediaTimeNow();
    ASSERT_EQ(mediaTimeNow, 0);

    // GetClockTimeNow
    auto clockTimeNow = syncManager_->GetMediaTimeNow();
    ASSERT_EQ(clockTimeNow, 0);

    // GetClockTime
    auto clockTime = syncManager_->GetClockTime(0);
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

// Scenario1: Test when syncer is nullptr then AddSynchronizer does nothing.
HWTEST_F(TestSyncManager, AddSynchronizer_ShouldDoNothing_WhenSyncerIsNull, TestSize.Level0)
{
    IMediaSynchronizer* syncer = nullptr;
    syncManager_->AddSynchronizer(syncer);
    // No exception should be thrown and no action should be performed.
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

// Scenario1: Test when syncer is nullptr then RemoveSynchronizer does nothing.
HWTEST_F(TestSyncManager, RemoveSynchronizer_ShouldDoNothing_WhenSyncerIsNull, TestSize.Level0)
{
    IMediaSynchronizer* syncer = nullptr;
    syncManager_->RemoveSynchronizer(syncer);
    // No exception should be thrown and nothing should change in syncManager
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

// Scenario3: Test case for rate greater than 0 and currentAbsMediaTime_ is HST_TIME_NONE
HWTEST_F(TestSyncManager,
    SetPlaybackRate_ShouldReturnOk_WhenRateGreaterThan0AndCurrentAbsMediaTimeIsNone, TestSize.Level0) {
    float rate = 1.0;
    syncManager_->currentAbsMediaTime_ = HST_TIME_NONE;
    EXPECT_EQ(syncManager_->SetPlaybackRate(rate), Status::OK);
}

// Scenario4: Test case for rate greater than 0 and currentAbsMediaTime_ is not HST_TIME_NONE
HWTEST_F(TestSyncManager,
    SetPlaybackRate_ShouldReturnOk_WhenRateGreaterThan0AndCurrentAbsMediaTimeIsNotNone, TestSize.Level0) {
    float rate = 1.0;
    syncManager_->currentAbsMediaTime_ = 100;
    EXPECT_EQ(syncManager_->SetPlaybackRate(rate), Status::OK);
}

// Scenario1: Test when inTime is less than minRangeStartOfMediaTime_
HWTEST_F(TestSyncManager, ClipMediaTime_ShouldClipToMin_WhenInTimeLessThanMin, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 100;
    syncManager_->maxRangeEndOfMediaTime_ = 200;
    int64_t inTime = 50;
    int64_t expected = 100;
    int64_t result = syncManager_->ClipMediaTime(inTime);
    ASSERT_EQ(expected, result);
}

// Scenario2: Test when inTime is more than maxRangeEndOfMediaTime_
HWTEST_F(TestSyncManager, ClipMediaTime_ShouldClipToMax_WhenInTimeMoreThanMax, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 100;
    syncManager_->maxRangeEndOfMediaTime_ = 200;
    int64_t inTime = 250;
    int64_t expected = 200;
    int64_t result = syncManager_->ClipMediaTime(inTime);
    ASSERT_EQ(expected, result);
}

// Scenario3: Test when inTime is between minRangeStartOfMediaTime_ and maxRangeEndOfMediaTime_
HWTEST_F(TestSyncManager, ClipMediaTime_ShouldNotClip_WhenInTimeBetweenMinAndMax, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 100;
    syncManager_->maxRangeEndOfMediaTime_ = 200;
    int64_t inTime = 150;
    int64_t expected = 150;
    int64_t result = syncManager_->ClipMediaTime(inTime);
    ASSERT_EQ(expected, result);
}

// Scenario4: Test when inTime is equal to minRangeStartOfMediaTime_
HWTEST_F(TestSyncManager, ClipMediaTime_ShouldNotClip_WhenInTimeEqualToMin, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 100;
    syncManager_->maxRangeEndOfMediaTime_ = 200;
    int64_t inTime = 100;
    int64_t expected = 100;
    int64_t result = syncManager_->ClipMediaTime(inTime);
    ASSERT_EQ(expected, result);
}

// Scenario5: Test when inTime is equal to maxRangeEndOfMediaTime_
HWTEST_F(TestSyncManager, ClipMediaTime_ShouldNotClip_WhenInTimeEqualToMax, TestSize.Level0)
{
    syncManager_->minRangeStartOfMediaTime_ = 100;
    syncManager_->maxRangeEndOfMediaTime_ = 200;
    int64_t inTime = 200;
    int64_t expected = 200;
    int64_t result = syncManager_->ClipMediaTime(inTime);
    ASSERT_EQ(expected, result);
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

// Scenario1: Test when playRate is 0 then return HST_TIME_NONE.
HWTEST_F(TestSyncManager, SimpleGetMediaTime_001, TestSize.Level0)
{
    int64_t anchorClockTime = 100;
    int64_t delayTime = 50;
    int64_t nowClockTime = 150;
    int64_t anchorMediaTime = 200;
    float playRate = 0;
    int64_t result =
        syncManager_->SimpleGetMediaTime(anchorClockTime, delayTime, nowClockTime, anchorMediaTime, playRate);
    ASSERT_EQ(result, HST_TIME_NONE);
}

// Scenario2: Test when any of the input parameters is HST_TIME_NONE then return HST_TIME_NONE.
HWTEST_F(TestSyncManager, SimpleGetMediaTime_002, TestSize.Level0)
{
    int64_t anchorClockTime = HST_TIME_NONE;
    int64_t delayTime = 50;
    int64_t nowClockTime = 150;
    int64_t anchorMediaTime = 200;
    float playRate = 1.0;
    int64_t result =
        syncManager_->SimpleGetMediaTime(anchorClockTime, delayTime, nowClockTime, anchorMediaTime, playRate);
    ASSERT_EQ(result, HST_TIME_NONE);
}

// Scenario3: Test when all parameters are valid then return anchorMediaTime.
HWTEST_F(TestSyncManager, SimpleGetMediaTime_003, TestSize.Level0)
{
    int64_t anchorClockTime = 100;
    int64_t delayTime = 50;
    int64_t nowClockTime = 150;
    int64_t anchorMediaTime = 200;
    float playRate = 1.0;
    int64_t result =
        syncManager_->SimpleGetMediaTime(anchorClockTime, delayTime, nowClockTime, anchorMediaTime, playRate);
    ASSERT_EQ(result, anchorMediaTime);
}

// Scenario1: Test when newMediaProgressTime is greater than or equal to lastReportMediaTime_
HWTEST_F(TestSyncManager, BoundMediaProgress_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.lastReportMediaTime_ = 100;
    int64_t newMediaProgressTime = 200;
    int64_t result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, newMediaProgressTime);
    ASSERT_EQ(mediaSyncManager.lastReportMediaTime_, newMediaProgressTime);
}

// Scenario2: Test when newMediaProgressTime is less than lastReportMediaTime_ and frameAfterSeeked_ is false
HWTEST_F(TestSyncManager, BoundMediaProgress_002, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.lastReportMediaTime_ = 200;
    mediaSyncManager.frameAfterSeeked_ = false;
    int64_t newMediaProgressTime = 100;
    int64_t result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, 200);
    ASSERT_EQ(mediaSyncManager.lastReportMediaTime_, 200);
}

// Scenario3: Test when newMediaProgressTime is less than lastReportMediaTime_ and frameAfterSeeked_ is true
HWTEST_F(TestSyncManager, BoundMediaProgress_003, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.lastReportMediaTime_ = 200;
    mediaSyncManager.frameAfterSeeked_ = true;
    int64_t newMediaProgressTime = 100;
    int64_t result = mediaSyncManager.BoundMediaProgress(newMediaProgressTime);
    ASSERT_EQ(result, newMediaProgressTime);
    ASSERT_EQ(mediaSyncManager.lastReportMediaTime_, newMediaProgressTime);
}

// Scenario1: Test case for when isSeeking_ is true
HWTEST_F(TestSyncManager, GetMediaTimeNow_001, TestSize.Level0)
{
    syncManager_->isSeeking_ = true;
    syncManager_->seekingMediaTime_ = 100;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 100);
}

// Scenario2: Test case for when clockState_ is PAUSED
HWTEST_F(TestSyncManager, GetMediaTimeNow_002, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedExactAbsMediaTime_ = 200;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 200);
}

// Scenario3: Test case for when SimpleGetMediaTimeExactly returns HST_TIME_NONE
HWTEST_F(TestSyncManager, GetMediaTimeNow_003, TestSize.Level0)
{
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 0);
}

HWTEST_F(TestSyncManager, GetMediaTimeNow_004, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedExactAbsMediaTime_ = 50;
    syncManager_->firstMediaTimeAfterSeek_ = 100;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 100);
}

// Scenario5: Test case for when startPts_ is not NONE
HWTEST_F(TestSyncManager, GetMediaTimeNow_005, TestSize.Level0)
{
    syncManager_->clockState_ = MediaSyncManager::State::PAUSED;
    syncManager_->pausedExactAbsMediaTime_ = 150;
    syncManager_->startPts_ = 50;
    EXPECT_EQ(syncManager_->GetMediaTimeNow(), 100);
}

// Scenario1: Test case for when clockState_ is PAUSED
HWTEST_F(TestSyncManager, GetClockTimeNow_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.clockState_ = MediaSyncManager::State::PAUSED;
    mediaSyncManager.GetClockTimeNow();
    EXPECT_EQ(mediaSyncManager.clockState_, MediaSyncManager::State::PAUSED);
}

// Scenario1: Test when playRate is 0, the function should return HST_TIME_NONE.
HWTEST_F(TestSyncManager, SimpleGetClockTime_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t anchorClockTime = 100;
    int64_t nowMediaTime = 200;
    int64_t anchorMediaTime = 150;
    float playRate = 0;
    int64_t result = mediaSyncManager.SimpleGetClockTime(anchorClockTime, nowMediaTime, anchorMediaTime, playRate);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario2: Test when any of the input parameters is HST_TIME_NONE, the function should return HST_TIME_NONE.
HWTEST_F(TestSyncManager, SimpleGetClockTime_002, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t anchorClockTime = HST_TIME_NONE;
    int64_t nowMediaTime = 200;
    int64_t anchorMediaTime = 150;
    float playRate = 1.0;
    int64_t result = mediaSyncManager.SimpleGetClockTime(anchorClockTime, nowMediaTime, anchorMediaTime, playRate);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario3: Test when all parameters are valid, the function should return the correct clock time.
HWTEST_F(TestSyncManager, SimpleGetClockTime_003, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    int64_t anchorClockTime = 100;
    int64_t nowMediaTime = 200;
    int64_t anchorMediaTime = 150;
    float playRate = 1.0;
    int64_t result = mediaSyncManager.SimpleGetClockTime(anchorClockTime, nowMediaTime, anchorMediaTime, playRate);
    EXPECT_EQ(result, 150);
}

HWTEST_F(TestSyncManager, GetClockTime_001, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.minRangeStartOfMediaTime_ = 100;
    mediaSyncManager.maxRangeEndOfMediaTime_ = 200;
    int64_t mediaTime = 50;
    int64_t result = mediaSyncManager.GetClockTime(mediaTime);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario2: Test when mediaTime is greater than maxRangeEndOfMediaTime_
HWTEST_F(TestSyncManager, GetClockTime_002, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.minRangeStartOfMediaTime_ = 100;
    mediaSyncManager.maxRangeEndOfMediaTime_ = 200;
    int64_t mediaTime = 250;
    int64_t result = mediaSyncManager.GetClockTime(mediaTime);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario3: Test when mediaTime is within the range
HWTEST_F(TestSyncManager, GetClockTime_003, TestSize.Level0)
{
    MediaSyncManager mediaSyncManager;
    mediaSyncManager.minRangeStartOfMediaTime_ = 100;
    mediaSyncManager.maxRangeEndOfMediaTime_ = 200;
    int64_t mediaTime = 150;
    int64_t result = mediaSyncManager.GetClockTime(mediaTime);
    EXPECT_EQ(result, HST_TIME_NONE);
}

// Scenario1: Test when supplier is nullptr then ReportPrerolled returns immediately.
HWTEST_F(TestSyncManager, ReportPrerolled_001, TestSize.Level0)
{
    syncManager_->ReportPrerolled(nullptr);
    // No further action is expected, as the function should return immediately.
}

// Scenario2: Test when allSyncerShouldPrerolled_ is false then ReportPrerolled returns immediately.
HWTEST_F(TestSyncManager, ReportPrerolled_002, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->allSyncerShouldPrerolled_ = false;
    syncManager_->ReportPrerolled(supplier);
    // No further action is expected, as the function should return immediately.
    delete supplier;
}

// Scenario3: Test when supplier is already in prerolledSyncers_ then ReportPrerolled returns immediately.
HWTEST_F(TestSyncManager, ReportPrerolled_003, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->allSyncerShouldPrerolled_ = true;
    syncManager_->prerolledSyncers_.emplace_back(supplier);
    syncManager_->ReportPrerolled(supplier);
    // No further action is expected, as the function should return immediately.
    delete supplier;
}

HWTEST_F(TestSyncManager, ReportPrerolled_004, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->allSyncerShouldPrerolled_ = true;
    syncManager_->ReportPrerolled(supplier);
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 1);
    EXPECT_EQ(syncManager_->prerolledSyncers_.front(), supplier);
    delete supplier;
}

// Scenario5: Test when all prerolledSyncers_ are equal to syncers_ then all prerolledSyncers_ are notified and cleared.
HWTEST_F(TestSyncManager, ReportPrerolled_005, TestSize.Level0)
{
    IMediaSynchronizer* supplier = new VideoSink();
    syncManager_->allSyncerShouldPrerolled_ = true;
    syncManager_->syncers_.emplace_back(supplier);
    syncManager_->ReportPrerolled(supplier);
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 0);
    EXPECT_EQ(syncManager_->prerolledSyncers_.size(), 0);
    delete supplier;
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS