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
#include "interstitial_vod_strategies.h"
#include "interstitial_live_strategies.h"
#include "mock/mock_media_demuxer.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

class InterstitialStrategiesUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    std::shared_ptr<VodScheduleStrategy> vodStrategy_;
    std::shared_ptr<LiveScheduleStrategy> liveStrategy_;
    std::shared_ptr<MockMediaDemuxer> mockDemuxer_;
};

void InterstitialStrategiesUnitTest::SetUpTestCase(void) {}
void InterstitialStrategiesUnitTest::TearDownTestCase(void) {}

void InterstitialStrategiesUnitTest::SetUp(void)
{
    vodStrategy_ = std::make_shared<VodScheduleStrategy>();
    liveStrategy_ = std::make_shared<LiveScheduleStrategy>();
    mockDemuxer_ = std::make_shared<MockMediaDemuxer>();
}

void InterstitialStrategiesUnitTest::TearDown(void)
{
    vodStrategy_ = nullptr;
    liveStrategy_ = nullptr;
    mockDemuxer_ = nullptr;
}

/**
 * @tc.name  : Test VodScheduleStrategy GetName
 * @tc.number: VodGetName_001
 * @tc.desc  : Test GetName returns "VodSchedule"
 */
HWTEST_F(InterstitialStrategiesUnitTest, VodGetName_001, TestSize.Level1)
{
    EXPECT_EQ(vodStrategy_->GetName(), "VodSchedule");
}

/**
 * @tc.name  : Test VodScheduleStrategy OnSeek
 * @tc.number: VodOnSeek_001
 * @tc.desc  : Test OnSeek marks events correctly
 */
HWTEST_F(InterstitialStrategiesUnitTest, VodOnSeek_001, TestSize.Level1)
{
    std::vector<TimedEventEntry> events;
    auto meta1 = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta1->id = "event_1";
    meta1->start = 3000;
    events.push_back({meta1, false});

    auto meta2 = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta2->id = "event_2";
    meta2->start = 8000;
    events.push_back({meta2, false});

    vodStrategy_->OnSeek(events, 5000);
    EXPECT_TRUE(events[0].notified);
    EXPECT_FALSE(events[1].notified);
}

/**
 * @tc.name  : Test VodScheduleStrategy OnSeek with null metadata
 * @tc.number: VodOnSeek_002
 * @tc.desc  : Test OnSeek handles null metadata
 */
HWTEST_F(InterstitialStrategiesUnitTest, VodOnSeek_002, TestSize.Level1)
{
    std::vector<TimedEventEntry> events;
    events.push_back({nullptr, false});

    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 3000;
    events.push_back({meta, false});

    vodStrategy_->OnSeek(events, 5000);
    EXPECT_TRUE(events[1].notified);
}

/**
 * @tc.name  : Test VodScheduleStrategy GetResumeSeekMs
 * @tc.number: VodGetResume_001
 * @tc.desc  : Test GetResumeSeekMs returns resumePointMs
 */
HWTEST_F(InterstitialStrategiesUnitTest, VodGetResume_001, TestSize.Level1)
{
    int64_t result = vodStrategy_->GetResumeSeekMs(5000, mockDemuxer_);
    EXPECT_EQ(result, 5000);
}

/**
 * @tc.name  : Test VodScheduleStrategy ShouldSaveResumePoint
 * @tc.number: VodShouldSave_001
 * @tc.desc  : Test ShouldSaveResumePoint returns true
 */
HWTEST_F(InterstitialStrategiesUnitTest, VodShouldSave_001, TestSize.Level1)
{
    EXPECT_TRUE(vodStrategy_->ShouldSaveResumePoint());
}

/**
 * @tc.name  : Test VodScheduleStrategy ShouldHandleSeek
 * @tc.number: VodShouldHandle_001
 * @tc.desc  : Test ShouldHandleSeek returns true
 */
HWTEST_F(InterstitialStrategiesUnitTest, VodShouldHandle_001, TestSize.Level1)
{
    EXPECT_TRUE(vodStrategy_->ShouldHandleSeek());
}

/**
 * @tc.name  : Test LiveScheduleStrategy GetName
 * @tc.number: LiveGetName_001
 * @tc.desc  : Test GetName returns "LiveSchedule"
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveGetName_001, TestSize.Level1)
{
    EXPECT_EQ(liveStrategy_->GetName(), "LiveSchedule");
}

/**
 * @tc.name  : Test LiveScheduleStrategy OnSeek
 * @tc.number: LiveOnSeek_001
 * @tc.desc  : Test OnSeek is no-op
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveOnSeek_001, TestSize.Level1)
{
    std::vector<TimedEventEntry> events;
    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 3000;
    events.push_back({meta, false});

    liveStrategy_->OnSeek(events, 5000);
    EXPECT_FALSE(events[0].notified);
}

/**
 * @tc.name  : Test LiveScheduleStrategy TrimExpiredEvents
 * @tc.number: LiveTrim_001
 * @tc.desc  : Test TrimExpiredEvents removes notified and expired events
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveTrim_001, TestSize.Level1)
{
    std::vector<TimedEventEntry> events;
    auto meta1 = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta1->id = "event_1";
    meta1->start = 3000;
    events.push_back({meta1, true});

    auto meta2 = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta2->id = "event_2";
    meta2->start = 8000;
    events.push_back({meta2, false});

    liveStrategy_->TrimExpiredEvents(events, 5000);
    EXPECT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].metadata->id, "event_2");
}

/**
 * @tc.name  : Test LiveScheduleStrategy TrimExpiredEvents
 * @tc.number: LiveTrim_002
 * @tc.desc  : Test TrimExpiredEvents removes null metadata
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveTrim_002, TestSize.Level1)
{
    std::vector<TimedEventEntry> events;
    events.push_back({nullptr, false});

    auto meta = std::make_shared<MediaAVCodec::AVTimedMetaData>();
    meta->id = "event_1";
    meta->start = 3000;
    events.push_back({meta, false});

    liveStrategy_->TrimExpiredEvents(events, 5000);
    EXPECT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].metadata->id, "event_1");
}

/**
 * @tc.name  : Test LiveScheduleStrategy GetResumeSeekMs
 * @tc.number: LiveGetResume_001
 * @tc.desc  : Test GetResumeSeekMs returns duration from demuxer
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveGetResume_001, TestSize.Level1)
{
    mockDemuxer_->duration_ = 10000000;
    int64_t result = liveStrategy_->GetResumeSeekMs(5000, mockDemuxer_);
    EXPECT_EQ(result, 10000);
}

/**
 * @tc.name  : Test LiveScheduleStrategy GetResumeSeekMs
 * @tc.number: LiveGetResume_002
 * @tc.desc  : Test GetResumeSeekMs returns 0 when demuxer is null
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveGetResume_002, TestSize.Level1)
{
    int64_t result = liveStrategy_->GetResumeSeekMs(5000, nullptr);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name  : Test LiveScheduleStrategy ShouldSaveResumePoint
 * @tc.number: LiveShouldSave_001
 * @tc.desc  : Test ShouldSaveResumePoint returns false
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveShouldSave_001, TestSize.Level1)
{
    EXPECT_FALSE(liveStrategy_->ShouldSaveResumePoint());
}

/**
 * @tc.name  : Test LiveScheduleStrategy ShouldHandleSeek
 * @tc.number: LiveShouldHandle_001
 * @tc.desc  : Test ShouldHandleSeek returns false
 */
HWTEST_F(InterstitialStrategiesUnitTest, LiveShouldHandle_001, TestSize.Level1)
{
    EXPECT_FALSE(liveStrategy_->ShouldHandleSeek());
}

} // namespace Media
} // namespace OHOS
