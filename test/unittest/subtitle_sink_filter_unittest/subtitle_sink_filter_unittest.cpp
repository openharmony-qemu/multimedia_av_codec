/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "subtitle_sink_filter_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;
static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;

void SubtitleSinkFilterUnitTest::SetUpTestCase(void) {}

void SubtitleSinkFilterUnitTest::TearDownTestCase(void) {}

void SubtitleSinkFilterUnitTest::SetUp(void)
{
    filter_ = std::make_shared<SubtitleSinkFilter>("testSubtitleSinkFilter");
}

void SubtitleSinkFilterUnitTest::TearDown(void)
{
    filter_ = nullptr;
}

/**
 * @tc.name  : Test OnBufferAvailable
 * @tc.number: OnBufferAvailable_001
 * @tc.desc  : Test sink == nullptr
 */
HWTEST_F(SubtitleSinkFilterUnitTest, OnBufferAvailable_001, TestSize.Level0)
{
    filter_ = nullptr;
    auto listener = std::make_shared<SubtitleSinkFilter::AVBufferAvailableListener>(filter_);
    listener->OnBufferAvailable();
    EXPECT_EQ(listener->subtitleSinkFilter_.lock(), nullptr);
}

/**
 * @tc.name  : Test DoUnFreeze
 * @tc.number: DoUnFreeze_001
 * @tc.desc  : Test state_ == FilterState::FROZEN && frameCnt_ > 0
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoUnFreeze_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    filter_->state_ = FilterState::FROZEN;
    filter_->frameCnt_ = NUM_1;
    auto ret = filter_->DoUnFreeze();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(filter_->state_, FilterState::RUNNING);
}

/**
 * @tc.name  : Test DoUnFreeze
 * @tc.number: DoUnFreeze_002
 * @tc.desc  : Test state_ != FilterState::FROZEN
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoUnFreeze_002, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    filter_->state_ = FilterState::ERROR;
    auto ret = filter_->DoUnFreeze();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(filter_->state_, FilterState::ERROR);
}

/**
 * @tc.name  : Test DoPause
 * @tc.number: DoPause_001
 * @tc.desc  : Test state_ == FilterState::FROZEN
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPause_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    filter_->state_ = FilterState::FROZEN;
    auto ret = filter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(filter_->state_, FilterState::PAUSED);
}

/**
 * @tc.name  : Test DoResume
 * @tc.number: DoResume_001
 * @tc.desc  : Test frameCnt_ > 0
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoResume_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    filter_->state_ = FilterState::PAUSED;
    filter_->frameCnt_ = NUM_1;
    auto ret = filter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(filter_->frameCnt_, NUM_0);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS