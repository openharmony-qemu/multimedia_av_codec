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

#include "video_sink_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int64_t WAITTIME_1 = 1000;
static const int64_t WAITTIME_2 = 50000;
static const int32_t DEFAULT_CAPACITY = 30;
static const double FRAMERATE = 1;
static const int32_t NUM_0 = 0;

void VideoSinkUnitTest::SetUpTestCase(void)
{
}

void VideoSinkUnitTest::TearDownTestCase(void)
{
}

void VideoSinkUnitTest::SetUp(void)
{
    sink_ = std::make_shared<VideoSink>();
}

void VideoSinkUnitTest::TearDown(void)
{
    sink_ = nullptr;
}

/**
 * @tc.name  : Test PerfRecord
 * @tc.number: PerfRecord_001
 * @tc.desc  : Test perfRecorder_.Record(Plugins::Us2Ms(waitTime)) == PerfRecorder::FULL
 */
HWTEST_F(VideoSinkUnitTest, PerfRecord_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    sink_->SetPerfRecEnabled(true);
    std::shared_ptr<EventReceiver> eventReceiver = std::make_shared<MockEventReceiver>();
    sink_->SetEventReceiver(eventReceiver);
    for (int i = 0; i < DEFAULT_CAPACITY; i++) {
        sink_->PerfRecord(WAITTIME_1);
    }
    sink_->PerfRecord(WAITTIME_1);
    EXPECT_EQ(sink_->perfRecorder_.list_.size(), 0);
}

/**
 * @tc.name  : Test RenderAtTimeLog
 * @tc.number: RenderAtTimeLog_001
 * @tc.desc  : Test enableRenderAtTime_ == nullptr
 */
HWTEST_F(VideoSinkUnitTest, RenderAtTimeLog_001, TestSize.Level0)
{
    struct VideoSinkTestable : public VideoSink {
        void SetEnableRenderAtTime(bool val) { enableRenderAtTime_ = val; }
    };
    VideoSinkTestable testSink;
    testSink.SetEnableRenderAtTime(false);
    EXPECT_EQ(testSink.enableRenderAtTime_, false);
    testSink.RenderAtTimeLog(WAITTIME_2);
}

/**
 * @tc.name  : Test InitWaitPeriod
 * @tc.number: InitWaitPeriod_001
 * @tc.desc  : Test initialVideoFrameRate > 1e-9
 */
HWTEST_F(VideoSinkUnitTest, InitWaitPeriod_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    syncCenter->SetInitialVideoFrameRate(FRAMERATE);
    sink_->SetSyncCenter(syncCenter);
    sink_->InitWaitPeriod();
    EXPECT_NE(sink_->initialVideoWaitPeriod_, NUM_0);
}

/**
 * @tc.name  : Test SetMediaMuted
 * @tc.number: SetMediaMuted
 * @tc.desc  : Test SetMediaMuted
 */
HWTEST_F(VideoSinkUnitTest, SetMediaMuted, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    sink_->isMuted_ = true;
    sink_->needSyncAfterMute_ = false;
    sink_->SetMediaMuted(false);
    EXPECT_EQ(sink_->needSyncAfterMute_, true);
    EXPECT_EQ(sink_->isMuted_, false);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
