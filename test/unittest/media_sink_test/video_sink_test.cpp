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

#include "gtest/gtest.h"
#include "filter/filter.h"
#include "video_sink.h"
#include "sink/media_synchronous_sink.h"
#include "media_sync_center_mock.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {
using namespace Pipeline;

class TestEventReceiver : public EventReceiver {
public:
    explicit TestEventReceiver()
    {
    }

    void OnEvent(const Event &event)
    {
        (void)event;
    }

private:
};

class TestVideoSink : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void) { }
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void) { }
    // SetUp: Called before each test cases
    void SetUp(void)
    {
        videoSink_ = std::make_shared<VideoSink>();
        ASSERT_TRUE(videoSink_ != nullptr);
    }
    // TearDown: Called after each test cases
    void TearDown(void)
    {
        videoSink_ = nullptr;
    }
public:
    std::shared_ptr<VideoSink> videoSink_ = nullptr;
};

HWTEST_F(TestVideoSink, do_sync_write_not_eos, TestSize.Level1)
{
    auto syncCenter = std::make_shared<MediaSyncManager>();
    ASSERT_TRUE(syncCenter != nullptr);
    videoSink_->SetSyncCenter(syncCenter);
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    ASSERT_TRUE(testEventReceiver != nullptr);
    videoSink_->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    auto setParam = videoSink_->SetParameter(meta);
    ASSERT_TRUE(setParam == Status::OK);
    videoSink_->ResetSyncInfo();
    videoSink_->SetLastPts(0, 0);
    videoSink_->SetFirstPts(HST_TIME_NONE);
    videoSink_->SetSeekFlag();
    uint64_t latency = 0;
    auto getLatency = videoSink_->GetLatency(latency);
    ASSERT_TRUE(getLatency == Status::OK);
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    buffer->flag_ = 0; // not eos
    int64_t actionClock = 0;
    videoSink_->DoSyncWrite(buffer, actionClock);
    buffer->flag_ = BUFFER_FLAG_EOS;
    videoSink_->DoSyncWrite(buffer, actionClock);
    buffer->pts_ = 1;
    videoSink_->lastBufferAnchoredClockTime_ = 1;
    videoSink_->seekFlag_ = false;
    (void)videoSink_->CheckBufferLatenessMayWait(buffer, 1);
    float speed = 0;
    videoSink_->AdjustPlaybackRate(speed);
}

HWTEST_F(TestVideoSink, do_sync_write_two_frames, TestSize.Level1)
{
    auto syncCenter = std::make_shared<MediaSyncManager>();
    ASSERT_TRUE(syncCenter != nullptr);
    videoSink_->SetSyncCenter(syncCenter);
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    ASSERT_TRUE(testEventReceiver != nullptr);
    videoSink_->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    auto setParam = videoSink_->SetParameter(meta);
    ASSERT_TRUE(setParam == Status::OK);
    videoSink_->ResetSyncInfo();
    videoSink_->SetLastPts(0, 0);
    videoSink_->SetFirstPts(HST_TIME_NONE);
    videoSink_->SetSeekFlag();
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    buffer->flag_ = 0; // not eos
    int64_t actionClock = 0;
    videoSink_->DoSyncWrite(buffer, actionClock);
    const std::shared_ptr<AVBuffer> buffer2 = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer2 != nullptr);
    buffer->flag_ = 0; // not eos
    videoSink_->DoSyncWrite(buffer2, actionClock);
    buffer->pts_ = 1;
    videoSink_->lastBufferAnchoredClockTime_ = 1;
    videoSink_->seekFlag_ = false;
    (void)videoSink_->CheckBufferLatenessMayWait(buffer, 1);
    float speed = 0;
    videoSink_->AdjustPlaybackRate(speed);
}

HWTEST_F(TestVideoSink, do_sync_write_eos, TestSize.Level1)
{
    auto syncCenter = std::make_shared<MediaSyncManager>();
    ASSERT_TRUE(syncCenter != nullptr);
    videoSink_->SetSyncCenter(syncCenter);
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    ASSERT_TRUE(testEventReceiver != nullptr);
    videoSink_->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    auto setParam = videoSink_->SetParameter(meta);
    ASSERT_TRUE(setParam == Status::OK);
    videoSink_->ResetSyncInfo();
    videoSink_->SetLastPts(0, 0);
    videoSink_->SetFirstPts(HST_TIME_NONE);
    videoSink_->SetSeekFlag();
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    buffer->flag_ = 1; // eos
    int64_t actionClock = 0;
    videoSink_->DoSyncWrite(buffer, actionClock);
    videoSink_->DoSyncWrite(buffer, actionClock);
    buffer->flag_ = BUFFER_FLAG_EOS;
    videoSink_->DoSyncWrite(buffer, actionClock);
    buffer->pts_ = 1;
    videoSink_->lastBufferAnchoredClockTime_ = 1;
    videoSink_->seekFlag_ = false;
    (void)videoSink_->CheckBufferLatenessMayWait(buffer, 1);
    float speed = 0;
    videoSink_->AdjustPlaybackRate(speed);
}

HWTEST_F(TestVideoSink, CheckBufferLatenessMayWait_001, TestSize.Level1)
{
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    auto buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    auto syncCenter = std::make_shared<MockMediaSyncCenter>();
    videoSink_->SetSyncCenter(syncCenter);

    syncCenter->returnInt64Queue_.push(Plugins::HST_TIME_NONE);
    bool result = videoSink_->CheckBufferLatenessMayWait(buffer, 1);
    EXPECT_EQ(result, 0);
}

HWTEST_F(TestVideoSink, CheckBufferLatenessMayWait_002, TestSize.Level1)
{
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    auto buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    auto syncCenter = std::make_shared<MockMediaSyncCenter>();
    videoSink_->SetSyncCenter(syncCenter);

    syncCenter->returnInt64Queue_.push(1000);
    syncCenter->returnInt64Queue_.push(2000);
    buffer->pts_ = 1500;
    bool result = videoSink_->CheckBufferLatenessMayWait(buffer, 2000);
    EXPECT_EQ(result, 0);
}

HWTEST_F(TestVideoSink, CheckBufferLatenessMayWait_003, TestSize.Level1)
{
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    auto buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    auto syncCenter = std::make_shared<MockMediaSyncCenter>();
    videoSink_->SetSyncCenter(syncCenter);

    syncCenter->returnInt64Queue_.push(1000);
    syncCenter->returnInt64Queue_.push(2000);
    buffer->pts_ = 1500;
    bool result = videoSink_->CheckBufferLatenessMayWait(buffer, 2000);
    EXPECT_EQ(result, false);
}

HWTEST_F(TestVideoSink, CheckBufferLatenessMayWait_004, TestSize.Level1)
{
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    auto buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    auto syncCenter = std::make_shared<MockMediaSyncCenter>();
    videoSink_->SetSyncCenter(syncCenter);
    videoSink_->lastBufferAnchoredClockTime_ = 1000;
    syncCenter->returnInt64Queue_.push(1000);
    syncCenter->returnInt64Queue_.push(2000);
    buffer->pts_ = 1500;
    bool result = videoSink_->CheckBufferLatenessMayWait(buffer, 2000);
    EXPECT_EQ(result, false);
}

// Scenario1: Test when firstPts_ is HST_TIME_NONE, SetFirstPts should set firstPts_ to pts.
HWTEST_F(TestVideoSink, SetFirstPts_ShouldSetFirstPts_WhenFirstPtsIsNone, TestSize.Level0)
{
    int64_t pts = 100;
    videoSink_->SetFirstPts(pts);
    EXPECT_EQ(videoSink_->firstPts_, pts);
}

// Scenario2: Test when firstPts_ is not HST_TIME_NONE, SetFirstPts should not change firstPts_.
HWTEST_F(TestVideoSink, SetFirstPts_ShouldNotChangeFirstPts_WhenFirstPtsIsNotNone, TestSize.Level0)
{
    int64_t pts = 100;
    videoSink_->firstPts_ = 200;
    videoSink_->SetFirstPts(pts);
    EXPECT_EQ(videoSink_->firstPts_, 200);
}

// Scenario1: Test when speed is 0.0f then AdjustPlaybackRate returns 1.0f.
HWTEST_F(TestVideoSink, AdjustPlaybackRate_ShouldReturn1_WhenSpeedIs0, TestSize.Level0)
{
    float speed = 0.0f;
    float result = videoSink_->AdjustPlaybackRate(speed);
    ASSERT_EQ(result, 1.0f);
}

// Scenario2: Test when speed is not 0.0f then AdjustPlaybackRate returns the same speed.
HWTEST_F(TestVideoSink, AdjustPlaybackRate_ShouldReturnSameSpeed_WhenSpeedIsNot0, TestSize.Level0)
{
    float speed = 0.5f;
    float result = videoSink_->AdjustPlaybackRate(speed);
    ASSERT_EQ(result, speed);
}

HWTEST_F(TestVideoSink, SetLastPts_001, TestSize.Level0)
{
    auto syncCenter = std::make_shared<MockMediaSyncCenter>();
    videoSink_->SetSyncCenter(syncCenter);
    syncCenter->returnInt64Queue_.push(987654321);
    int64_t lastPts = 123456789;
    videoSink_->SetLastPts(lastPts, 0);
    EXPECT_EQ(videoSink_->lastPts_, lastPts);
}

HWTEST_F(TestVideoSink, SetLastPts_002, TestSize.Level0)
{
    auto syncCenter = std::make_shared<MockMediaSyncCenter>();
    videoSink_->SetSyncCenter(nullptr);
    int64_t lastPts = 123456789;
    videoSink_->SetLastPts(lastPts, 0);
    EXPECT_EQ(videoSink_->lastPts_, lastPts);
}

HWTEST_F(TestVideoSink, ReportPts_001, TestSize.Level0)
{
    int64_t nowPts = 0;
    int64_t lastPts = 2;
    videoSink_->SetLastPts(lastPts, 0);
    EXPECT_EQ(videoSink_->lastPts_, lastPts);
    videoSink_->ReportPts(nowPts);
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS