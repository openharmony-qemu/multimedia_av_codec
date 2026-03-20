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

#include "subtitle_sink_unittest.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const string TEXT_A = "a";
static const string TEXT_B = "b";
static const int64_t PTR_100 = 100;
static const int64_t PTR_200 = 200;
static const int64_t DURATION_50 = 50;
static const int64_t TIME_50 = 50;
static const int64_t TIME_220 = 220;
static const int64_t TIME_300 = 300;
static const uint32_t RET_0 = 0;
static const uint32_t RET_1 = 1;
static const uint32_t RET_2 = 2;
static const int32_t ERROR_RET = 0;

void SubtitleSinkUnitTest::SetUpTestCase(void)
{
}

void SubtitleSinkUnitTest::TearDownTestCase(void)
{
}

void SubtitleSinkUnitTest::SetUp(void)
{
    sink_ = std::make_shared<SubtitleSink>();
}

void SubtitleSinkUnitTest::TearDown(void)
{
    sink_ = nullptr;
}

/**
 * @tc.name  : Test GetTargetSubtitleIndex
 * @tc.number: GetTargetSubtitleIndex_001
 * @tc.desc  : Test startTime > currentTime
 */
HWTEST_F(SubtitleSinkUnitTest, GetTargetSubtitleIndex_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    sink_->GetTargetSubtitleIndex(TIME_50);
    EXPECT_EQ(sink_->currentInfoIndex_, RET_0);
}

/**
 * @tc.name  : Test GetTargetSubtitleIndex
 * @tc.number: GetTargetSubtitleIndex_002
 * @tc.desc  : Test endTime < currentTime
 */
HWTEST_F(SubtitleSinkUnitTest, GetTargetSubtitleIndex_002, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    sink_->GetTargetSubtitleIndex(TIME_300);
    EXPECT_EQ(sink_->currentInfoIndex_, RET_2);
}

/**
 * @tc.name  : Test GetTargetSubtitleIndex
 * @tc.number: GetTargetSubtitleIndex_003
 * @tc.desc  : Test startTime <= currentTime <= endTime
 */
HWTEST_F(SubtitleSinkUnitTest, GetTargetSubtitleIndex_003, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    sink_->GetTargetSubtitleIndex(TIME_220);
    EXPECT_EQ(sink_->currentInfoIndex_, RET_1);
}

/**
 * @tc.name  : Test ResetSyncInfo
 * @tc.number: ResetSyncInfo_001
 * @tc.desc  : Test syncCenter != nullptr
 */
HWTEST_F(SubtitleSinkUnitTest, ResetSyncInfo_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    sink_->SetSyncCenter(syncCenter);
    EXPECT_CALL(*syncCenter, AddSynchronizer(_)).WillRepeatedly(Return());
    sink_->ResetSyncInfo();
    EXPECT_CALL(*syncCenter, Reset()).WillRepeatedly(Return(Status::OK));
    EXPECT_EQ(syncCenter->isFrameAfterSeeked_, false);
}

/**
 * @tc.name  : Test GetMediaTime
 * @tc.number: GetMediaTime_001
 * @tc.desc  : Test syncCenter == nullptr
 */
HWTEST_F(SubtitleSinkUnitTest, GetMediaTime_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    sink_->SetSyncCenter(nullptr);
    auto ret = sink_->GetMediaTime();
    EXPECT_EQ(ret, ERROR_RET);
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_001
 * @tc.desc  : Test Flush with isSeekFlush=true and empty subtitleInfoVec
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    auto ret = sink_->Flush(true);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
    EXPECT_TRUE(sink_->subtitleInfoVec_.empty());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_002
 * @tc.desc  : Test Flush with isSeekFlush=false and empty subtitleInfoVec
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_002, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    auto ret = sink_->Flush(false);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
    EXPECT_TRUE(sink_->subtitleInfoVec_.empty());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_003
 * @tc.desc  : Test Flush with isSeekFlush=true and non-empty subtitleInfoVec
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_003, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    
    auto ret = sink_->Flush(true);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
    EXPECT_TRUE(sink_->subtitleInfoVec_.empty());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_004
 * @tc.desc  : Test Flush with isSeekFlush=false and non-empty subtitleInfoVec
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_004, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    sink_->SetSyncCenter(syncCenter);
    
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    
    auto ret = sink_->Flush(false);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
    EXPECT_FALSE(sink_->subtitleInfoVec_.empty());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_005
 * @tc.desc  : Test Flush with isSeekFlush=false and expired subtitles
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_005, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    sink_->SetSyncCenter(syncCenter);
    
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    
    auto ret = sink_->Flush(false);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_006
 * @tc.desc  : Test Flush with isSeekFlush=false and mixed expired/active subtitles
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_006, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    sink_->SetSyncCenter(syncCenter);
    
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    
    auto ret = sink_->Flush(false);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
    EXPECT_EQ(sink_->subtitleInfoVec_.size(), RET_2);
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_007
 * @tc.desc  : Test Flush with null inputBufferQueueConsumer
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_007, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    
    auto ret = sink_->Flush(true);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_008
 * @tc.desc  : Test Flush with isSeekFlush=true clears all subtitles
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_008, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    auto syncCenter = std::make_shared<Pipeline::MediaSyncManager>();
    sink_->SetSyncCenter(syncCenter);
    
    sink_->subtitleInfoVec_.emplace_back(TEXT_A, PTR_100, DURATION_50);
    sink_->subtitleInfoVec_.emplace_back(TEXT_B, PTR_200, DURATION_50);
    
    auto ret = sink_->Flush(true);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->isFlush_.load());
    EXPECT_TRUE(sink_->subtitleInfoVec_.empty());
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_009
 * @tc.desc  : Test Flush sets shouldUpdate flag
 */
HWTEST_F(SubtitleSinkUnitTest, Flush_009, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto meta = std::make_shared<Meta>();
    sink_->Init(meta, nullptr);
    sink_->Prepare();
    
    sink_->shouldUpdate_ = false;
    
    auto ret = sink_->Flush(true);
    
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(sink_->shouldUpdate_.load());
}
} // namespace Media
} // namespace OHOS