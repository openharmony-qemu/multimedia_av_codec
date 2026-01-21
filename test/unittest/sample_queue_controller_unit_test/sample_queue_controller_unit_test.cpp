/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "sample_queue_controller_unit_test.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

using namespace OHOS;
using namespace OHOS::Media;

namespace {
constexpr uint32_t S_TO_US = 1000 * 1000;
constexpr double MIN_FIRST_DURATION = 0;
constexpr double MAX_FIRST_DURATION = 20;
constexpr uint64_t MIN_DURATION = 1;
constexpr uint64_t MAX_DURATION = 20;
constexpr uint64_t START_CONSUME_WATER_LOOP = 5 * 1000 * 1000;
}

namespace OHOS::Media {
namespace Test {
void SampleQueueControllerUnitTest::SetUpTestCase(void)
{}

void SampleQueueControllerUnitTest::TearDownTestCase(void)
{}

void SampleQueueControllerUnitTest::SetUp()
{
    sampleQueueController_ = std::make_shared<SampleQueueController>();
}

void SampleQueueControllerUnitTest::TearDown()
{}

/**
 * @tc.name  : TEST_BUFFERING_DURATION
 * @tc.number: 001
 * @tc.desc  : Test bufferingDuration_ firstBufferingDuration_
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_BUFFERING_DURATION, TestSize.Level1)
{
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), START_CONSUME_WATER_LOOP);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    // case duration zero
    auto strategy = std::make_shared<Plugins::PlayStrategy>();
    strategy->bufferDurationForPlaying = 3;
    strategy->duration = 0;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 3 * S_TO_US);
    EXPECT_EQ(sampleQueueController_->firstBufferingDuration_, 3 * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    EXPECT_EQ(sampleQueueController_->bufferingDuration_, 0);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), START_CONSUME_WATER_LOOP);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    // case duration min
    strategy->duration = 1;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 3 * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    EXPECT_EQ(sampleQueueController_->bufferingDuration_, static_cast<uint64_t>(MIN_DURATION) * S_TO_US);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), static_cast<uint64_t>(MIN_DURATION) * S_TO_US);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    // case duration max
    strategy->duration = 99;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 3 * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    EXPECT_EQ(sampleQueueController_->bufferingDuration_, static_cast<uint64_t>(MAX_DURATION) * S_TO_US);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), static_cast<uint64_t>(MAX_DURATION) * S_TO_US);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    // case duration normal
    strategy->duration = 10;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 3 * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    EXPECT_EQ(sampleQueueController_->bufferingDuration_, 10 * S_TO_US);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 10 * S_TO_US);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);
}

/**
 * @tc.name  : TEST_BUFFERING_DURATION_FOR_PLAYING
 * @tc.number: 002
 * @tc.desc  : Test bufferingDuration_ firstBufferingDuration_
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_BUFFERING_DURATION_FOR_PLAYING, TestSize.Level1)
{
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), START_CONSUME_WATER_LOOP);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    auto strategy = std::make_shared<Plugins::PlayStrategy>();

    // case duration for playing normal
    strategy->duration = 10;
    strategy->bufferDurationForPlaying = 5;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 5 * S_TO_US);
    EXPECT_EQ(sampleQueueController_->firstBufferingDuration_, 5 * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    EXPECT_EQ(sampleQueueController_->bufferingDuration_, 10 * S_TO_US);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 10 * S_TO_US);
    EXPECT_EQ(sampleQueueController_->bufferingDuration_, 10 * S_TO_US);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    // case duration for playing max
    strategy->bufferDurationForPlaying = 99;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), MAX_FIRST_DURATION * S_TO_US);
    EXPECT_EQ(sampleQueueController_->firstBufferingDuration_, MAX_FIRST_DURATION * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 10 * S_TO_US);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);

    // case duration for playing min
    strategy->bufferDurationForPlaying = -1;
    sampleQueueController_->SetBufferingDuration(strategy);
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), MIN_FIRST_DURATION * S_TO_US);
    EXPECT_EQ(sampleQueueController_->firstBufferingDuration_, MIN_FIRST_DURATION * S_TO_US);
    EXPECT_TRUE(sampleQueueController_->isSetFirstBufferingDuration_);
    sampleQueueController_->DisableFirstBufferingDuration();
    EXPECT_EQ(sampleQueueController_->GetBufferingDuration(), 10 * S_TO_US);
    EXPECT_FALSE(sampleQueueController_->isSetFirstBufferingDuration_);
}
}
}  // namespace OHOS::Media