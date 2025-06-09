/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "video_sink_mock_test.h"
 
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
namespace Test {
using namespace Pipeline;
 
HWTEST_F(TestVideoSinkMock, Test_ResetSyncInfo, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    videoSink_->ResetSyncInfo();
    EXPECT_EQ(videoSink_->hasReportedPreroll_, false);
    EXPECT_EQ(videoSink_->renderFrameCnt_, 0);
    EXPECT_EQ(videoSink_->dropFrameContinuouslyCnt_, 0);
    EXPECT_EQ(videoSink_->isFirstFrame_, true);
    EXPECT_EQ(videoSink_->lastBufferRelativePts_, HST_TIME_NONE);
    EXPECT_EQ(videoSink_->lastBufferAnchoredClockTime_, HST_TIME_NONE);
    EXPECT_EQ(videoSink_->seekFlag_, false);
    EXPECT_EQ(videoSink_->lastPts_, HST_TIME_NONE);
    EXPECT_EQ(videoSink_->lastClockTime_, HST_TIME_NONE);
    EXPECT_EQ(videoSink_->needUpdateTimeAnchor_, true);
    EXPECT_EQ(videoSink_->lagDetector_.lagTimes_, 0);
    EXPECT_EQ(videoSink_->lagDetector_.maxLagDuration_, 0);
    EXPECT_EQ(videoSink_->lagDetector_.lastSystemTimeMs_, 0);
    EXPECT_EQ(videoSink_->lagDetector_.lastBufferTimeMs_, 0);
    EXPECT_EQ(videoSink_->lagDetector_.totalLagDuration_, 0);
    EXPECT_TRUE(videoSink_->perfRecorder_.list_.empty());
}
 
HWTEST_F(TestVideoSinkMock, Test_GetLatency, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    uint64_t latencyNs = 0;
    EXPECT_EQ(videoSink_->GetLatency(latencyNs), Status::OK);
    EXPECT_EQ(latencyNs, 10); // default video latency is 10 ns.
}
 
HWTEST_F(TestVideoSinkMock, Test_SetSyncCenter, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    mockSyncCenter_->synchronizerAdded_ = false;
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    EXPECT_TRUE(mockSyncCenter_->synchronizerAdded_);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetEventReceiver, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockEventReceiver_ != nullptr);
    videoSink_->eventReceiver_ = nullptr;
    videoSink_->lagDetector_.eventReceiver_ = nullptr;
    videoSink_->SetEventReceiver(mockEventReceiver_);
    EXPECT_NE(videoSink_->eventReceiver_, nullptr);
    EXPECT_NE(videoSink_->lagDetector_.eventReceiver_, nullptr);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetFirstPts_AlreadySet, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    videoSink_->firstPts_ = 1000; // assume first pts is already set to 1000
    mockSyncCenter_->startPtsGetTime_ = 0;
    videoSink_->SetFirstPts(100); // assume current pts is 100
    EXPECT_EQ(mockSyncCenter_->startPtsGetTime_, 0); // GetMediaStartPts is expected not called
    EXPECT_EQ(videoSink_->firstPts_, 1000); // firstPts_ not changed, remains 1000
}
 
HWTEST_F(TestVideoSinkMock, Test_SetFirstPts_FirstSet_NoSyncCenter, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    videoSink_->syncCenter_.reset();
    videoSink_->firstPts_ = HST_TIME_NONE;
    mockSyncCenter_->startPtsGetTime_ = 0;
    videoSink_->SetFirstPts(100); // assume current pts is 100
    EXPECT_EQ(mockSyncCenter_->startPtsGetTime_, 0); // GetMediaStartPts is expected not called
    EXPECT_EQ(videoSink_->firstPts_, 100); // firstPts_ changed, set to 100;
}
 
HWTEST_F(TestVideoSinkMock, Test_SetFirstPts_FirstSet_HaveGlobalStart, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    videoSink_->firstPts_ = HST_TIME_NONE;
    mockSyncCenter_->startPtsGetTime_ = 0;
    mockSyncCenter_->returnInt64Queue_.push(1000); // assume golbal startPts is 1000
    mockSyncCenter_->returnInt64Queue_.push(1000); // assume golbal startPts is 1000
    videoSink_->SetFirstPts(100); // assume current pts is 100
    EXPECT_EQ(mockSyncCenter_->startPtsGetTime_, 2); // GetMediaStartPts is expected to be called 2 times
    EXPECT_EQ(videoSink_->firstPts_, 1000); // firstPts_ not changed, remained 1000;
}
 
HWTEST_F(TestVideoSinkMock, Test_SetFirstPts_FirstSet_NoGlobalStart, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    videoSink_->firstPts_ = HST_TIME_NONE;
    mockSyncCenter_->startPtsGetTime_ = 0;
    mockSyncCenter_->returnInt64Queue_.push(HST_TIME_NONE); // assume golbal startPts is not set
    mockSyncCenter_->startPts_ = HST_TIME_NONE;
    videoSink_->SetFirstPts(100); // assume current pts is 100
    EXPECT_EQ(mockSyncCenter_->startPtsGetTime_, 1); // GetMediaStartPts is expected to be called 2 times
    EXPECT_EQ(videoSink_->firstPts_, 100); // firstPts_ changed, set to 100;
}
 
HWTEST_F(TestVideoSinkMock, Test_SetSeekFlag, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    videoSink_->seekFlag_ = false;
    videoSink_->SetSeekFlag();
    EXPECT_TRUE(videoSink_->seekFlag_);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetLastPts_WithSyncCenter_Case1, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    videoSink_->lastPts_ = HST_TIME_NONE;
    videoSink_->lastClockTime_ = HST_TIME_NONE;
    int64_t lastPts = 1000; // 1000
    int64_t renderDelay = 100; // 100
    int64_t clockNow = 10000; // 10000
    mockSyncCenter_->returnInt64Queue_.push(clockNow);
    videoSink_->SetLastPts(lastPts, renderDelay);
    EXPECT_EQ(videoSink_->lastPts_, lastPts);
    EXPECT_EQ(videoSink_->lastClockTime_, clockNow + renderDelay);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetLastPts_WithSyncCenter_Case2, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    videoSink_->lastPts_ = HST_TIME_NONE;
    videoSink_->lastClockTime_ = HST_TIME_NONE;
    int64_t lastPts = 1000; // 1000
    int64_t renderDelay = -100; // -100
    int64_t clockNow = 10000; // 10000
    mockSyncCenter_->returnInt64Queue_.push(clockNow);
    videoSink_->SetLastPts(lastPts, renderDelay);
    EXPECT_EQ(videoSink_->lastPts_, lastPts);
    EXPECT_EQ(videoSink_->lastClockTime_, clockNow);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetLastPts_NoSyncCenter, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    videoSink_->syncCenter_.reset();
    videoSink_->lastPts_ = HST_TIME_NONE;
    videoSink_->lastClockTime_ = HST_TIME_NONE;
    int64_t lastPts = 1000; // 1000
    int64_t renderDelay = 100; // 100
    videoSink_->SetLastPts(lastPts, renderDelay);
    EXPECT_EQ(videoSink_->lastPts_, lastPts);
    EXPECT_EQ(videoSink_->lastClockTime_, HST_TIME_NONE);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetParameter_WithSyncCenter_Case1, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    int64_t trackStartTime = 100; // 100
    int32_t trackId = 1; // 1
    int64_t trackDuration = 1100; // 1000
    meta->Set<Tag::MEDIA_START_TIME>(trackStartTime);
    meta->Set<Tag::REGULAR_TRACK_ID>(trackId);
    meta->Set<Tag::MEDIA_DURATION>(trackDuration);
    EXPECT_EQ(videoSink_->SetParameter(meta), Status::OK);
    EXPECT_EQ(mockSyncCenter_->setMediaRangeStartTime_, 1); // expected called once
    EXPECT_EQ(mockSyncCenter_->setMediaRangeEndTime_, 1); // expected called once
    EXPECT_EQ(mockSyncCenter_->mediaRangeEndValue_, trackStartTime + trackDuration);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetParameter_WithSyncCenter_Case2, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    int64_t trackStartTime = 100; // 100
    int32_t trackId = 1; // 1
    meta->Set<Tag::MEDIA_START_TIME>(trackStartTime);
    meta->Set<Tag::REGULAR_TRACK_ID>(trackId);
    EXPECT_EQ(videoSink_->SetParameter(meta), Status::OK);
    EXPECT_EQ(mockSyncCenter_->setMediaRangeStartTime_, 1); // expected called once
    EXPECT_EQ(mockSyncCenter_->setMediaRangeEndTime_, 1); // expected called once
    EXPECT_EQ(mockSyncCenter_->mediaRangeEndValue_, INT64_MAX);
}
 
HWTEST_F(TestVideoSinkMock, Test_SetParameter_NoSyncCenter_Case1, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->syncCenter_.reset();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    int64_t trackStartTime = 100; // 100
    int32_t trackId = 1; // 1
    int64_t trackDuration = 1100; // 1000
    meta->Set<Tag::MEDIA_START_TIME>(trackStartTime);
    meta->Set<Tag::REGULAR_TRACK_ID>(trackId);
    meta->Set<Tag::MEDIA_DURATION>(trackDuration);
    EXPECT_EQ(videoSink_->SetParameter(meta), Status::OK);
    EXPECT_EQ(mockSyncCenter_->setMediaRangeStartTime_, 0); // expect called 0 time
    EXPECT_EQ(mockSyncCenter_->setMediaRangeEndTime_, 0); // expect called 0 time
    EXPECT_EQ(mockSyncCenter_->mediaRangeEndValue_, 0); // default value 0
}
 
HWTEST_F(TestVideoSinkMock, Test_SetParameter_NoSyncCenter_Case2, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->syncCenter_.reset();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    int64_t trackStartTime = 101; // 101
    int32_t trackId = 2; // 2
    meta->Set<Tag::MEDIA_START_TIME>(trackStartTime);
    meta->Set<Tag::REGULAR_TRACK_ID>(trackId);
    EXPECT_EQ(videoSink_->SetParameter(meta), Status::OK);
    EXPECT_EQ(mockSyncCenter_->setMediaRangeStartTime_, 0); // expect called 0 time
    EXPECT_EQ(mockSyncCenter_->setMediaRangeEndTime_, 0); // expect called 0 time
    EXPECT_EQ(mockSyncCenter_->mediaRangeEndValue_, 0); // default value 0
}
 
HWTEST_F(TestVideoSinkMock, Test_UpdateTimeAnchorActually_Case1, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(buffer != nullptr);
    buffer->pts_ = 1000; // 1000
    int64_t renderDelay = 100; // 100
    int64_t clockNow = 10000; // 10000
    mockSyncCenter_->returnInt64Queue_.push(clockNow);
    videoSink_->UpdateTimeAnchorActually(buffer, renderDelay);
    EXPECT_EQ(mockSyncCenter_->setLastVideoBufferPtsTimes_, 1); // called 1 time
    EXPECT_EQ(mockSyncCenter_->updateTimeAnchorTimes_, 1); // called 1 time
    EXPECT_EQ(mockSyncCenter_->lastTimeAnchorClock_, clockNow + renderDelay);
}
 
HWTEST_F(TestVideoSinkMock, Test_UpdateTimeAnchorActually_Case2, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    ASSERT_TRUE(mockSyncCenter_ != nullptr);
    videoSink_->SetSyncCenter(std::dynamic_pointer_cast<MediaSyncManager>(mockSyncCenter_));
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    ASSERT_TRUE(buffer != nullptr);
    buffer->pts_ = 1000; // 1000
    int64_t renderDelay = -100; // -100
    int64_t clockNow = 10000; // 10000
    mockSyncCenter_->returnInt64Queue_.push(clockNow);
    videoSink_->UpdateTimeAnchorActually(buffer, renderDelay);
    EXPECT_EQ(mockSyncCenter_->setLastVideoBufferPtsTimes_, 1); // called 1 time
    EXPECT_EQ(mockSyncCenter_->updateTimeAnchorTimes_, 1); // called 1 time
    EXPECT_EQ(mockSyncCenter_->lastTimeAnchorClock_, clockNow);
}
 
HWTEST_F(TestVideoSinkMock, Test_GetLagInfo_Case1, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    constexpr int32_t LAG_TIMES = 2; // 2
    constexpr int32_t MAX_LAG_DURATION = 100; // 100
    constexpr int32_t TOTAL_LAG_TURATION = 120; // 100
    videoSink_->lagDetector_.lagTimes_ = LAG_TIMES;
    videoSink_->lagDetector_.maxLagDuration_ = MAX_LAG_DURATION;
    videoSink_->lagDetector_.totalLagDuration_ = TOTAL_LAG_TURATION;
    int32_t lagTimes = -1;
    int32_t maxLagDuration = -1;
    int32_t avgLagDuration = -1;
    EXPECT_EQ(videoSink_->GetLagInfo(lagTimes, maxLagDuration, avgLagDuration), Status::OK);
    EXPECT_EQ(lagTimes, LAG_TIMES);
    EXPECT_EQ(maxLagDuration, MAX_LAG_DURATION);
    EXPECT_EQ(avgLagDuration, TOTAL_LAG_TURATION / LAG_TIMES);
}
 
HWTEST_F(TestVideoSinkMock, Test_GetLagInfo_Case2, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    constexpr int32_t LAG_TIMES = 0; // 0
    constexpr int32_t MAX_LAG_DURATION = 0; // 0
    constexpr int32_t TOTAL_LAG_TURATION = 0; // 0
    videoSink_->lagDetector_.lagTimes_ = LAG_TIMES;
    videoSink_->lagDetector_.maxLagDuration_ = MAX_LAG_DURATION;
    videoSink_->lagDetector_.totalLagDuration_ = TOTAL_LAG_TURATION;
    int32_t lagTimes = -1;
    int32_t maxLagDuration = -1;
    int32_t avgLagDuration = -1;
    EXPECT_EQ(videoSink_->GetLagInfo(lagTimes, maxLagDuration, avgLagDuration), Status::OK);
    EXPECT_EQ(lagTimes, LAG_TIMES);
    EXPECT_EQ(maxLagDuration, MAX_LAG_DURATION);
    EXPECT_EQ(avgLagDuration, 0); // no average
}
 
HWTEST_F(TestVideoSinkMock, Test_SetPerfRecEnabled, TestSize.Level1)
{
    ASSERT_TRUE(videoSink_ != nullptr);
    videoSink_->isPerfRecEnabled_ = false;
    EXPECT_EQ(videoSink_->SetPerfRecEnabled(true), Status::OK);
    EXPECT_TRUE(videoSink_->isPerfRecEnabled_);
    EXPECT_EQ(videoSink_->SetPerfRecEnabled(false), Status::OK);
    EXPECT_FALSE(videoSink_->isPerfRecEnabled_);
}
 
} // namespace Test
} // namespace Media
} // namespace OHOS