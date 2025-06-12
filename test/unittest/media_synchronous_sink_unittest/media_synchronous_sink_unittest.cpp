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

#include "media_synchronous_sink_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int64_t INVALID_Time = -1;

void MediaSynchronousSinkUnitTest::SetUpTestCase(void)
{
}

void MediaSynchronousSinkUnitTest::TearDownTestCase(void)
{
}

void MediaSynchronousSinkUnitTest::SetUp(void)
{
    sink_ = std::make_shared<VideoSink>();
}

void MediaSynchronousSinkUnitTest::TearDown(void)
{
    sink_ = nullptr;
}

/**
 * @tc.name  : Test WriteToPluginRefTimeSync
 * @tc.number: WriteToPluginRefTimeSync_001
 * @tc.desc  : Test hasReportedPreroll_ == false
 *             Test syncCenter_ != nullptr
 *             Test waitForPrerolled_ == true
 *             Test waitPrerolledTimeout_ == -1
 */
HWTEST_F(MediaSynchronousSinkUnitTest, WriteToPluginRefTimeSync_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    // Test hasReportedPreroll_ == false
    sink_->hasReportedPreroll_ = false;
    // Test waitForPrerolled_ == true
    sink_->waitForPrerolled_ = true;
    // Test syncCenter_ != nullptr
    auto syncCenter_ = std::make_shared<MediaSyncManager>();
    sink_->SetSyncCenter(syncCenter_);
    std::shared_ptr<OHOS::Media::AVBuffer> buffer = nullptr;
    // Test waitPrerolledTimeout_ == -1
    sink_->waitPrerolledTimeout_ = INVALID_Time;
    sink_->WriteToPluginRefTimeSync(buffer);
    EXPECT_TRUE(sink_->hasReportedPreroll_);
    EXPECT_FALSE(sink_->waitForPrerolled_);
}

/**
 * @tc.name  : Test OnInterrupted
 * @tc.number: OnInterrupted_001
 * @tc.desc  : Test default
 */
HWTEST_F(MediaSynchronousSinkUnitTest, OnInterrupted_001, TestSize.Level0)
{
    ASSERT_NE(sink_, nullptr);
    bool isInterruptNeeded = true;
    sink_->OnInterrupted(isInterruptNeeded);
    EXPECT_EQ(sink_->isInterruptNeeded_, isInterruptNeeded);
    EXPECT_EQ(sink_->waitForPrerolled_, false);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS
