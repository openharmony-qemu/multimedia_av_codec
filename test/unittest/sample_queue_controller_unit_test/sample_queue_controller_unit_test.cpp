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

/**
 * @tc.name  : TEST_GETQUEUESIZE
 * @tc.number: 001
 * @tc.desc  : Test function GetQueueSize
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_GETQUEUESIZE_01, TestSize.Level1)
{
    sampleQueueController_->queueSizeMap_.clear();
    sampleQueueController_->SetQueueSize(101, 10);
    uint64_t ret = sampleQueueController_->GetQueueSize(101);
    EXPECT_EQ(ret, SampleQueueController::QUEUE_SIZE_MIN);
}

/**
 * @tc.name  : TEST_GETQUEUESIZE
 * @tc.number: 002
 * @tc.desc  : Test function GetQueueSize
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_GETQUEUESIZE_02, TestSize.Level1)
{
    sampleQueueController_->queueSizeMap_.clear();
    sampleQueueController_->SetQueueSize(101, 100);
    uint64_t ret = sampleQueueController_->GetQueueSize(101);
    EXPECT_EQ(ret, 100);
}

/**
 * @tc.name  : TEST_ProduceIncrementFrameCount
 * @tc.number: 001
 * @tc.desc  : Test function ProduceIncrementFrameCount
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ProduceIncrementFrameCount, TestSize.Level1)
{
    sampleQueueController_->produceSpeedCountInfo_.clear();
    sampleQueueController_->produceSpeedCountInfo_[101] = std::make_shared<SpeedCountInfo>();
    sampleQueueController_->ProduceIncrementFrameCount(101);
    sampleQueueController_->produceSpeedCountInfo_.erase(101);
    sampleQueueController_->ProduceIncrementFrameCount(101);
    std::shared_ptr<SpeedCountInfo> ret = sampleQueueController_->produceSpeedCountInfo_[101];
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name  : TEST_ShouldStopConsume
 * @tc.number: 001
 * @tc.desc  : Test function ShouldStopConsume
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopConsume_01, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = nullptr;
    std::unique_ptr<Task> task = nullptr;
    bool ret = true;
    ret = sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopConsume
 * @tc.number: 002
 * @tc.desc  : Test function ShouldStopConsume
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopConsume_02, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = nullptr;
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopConsume
 * @tc.number: 003
 * @tc.desc  : Test function ShouldStopConsume
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopConsume_03, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    std::unique_ptr<Task> task = nullptr;
    bool ret = true;
    ret = sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopConsume
 * @tc.number: 004
 * @tc.desc  : Test function ShouldStopConsume
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopConsume_04, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->UpdateLastEnterSamplePts(200);
    sampleQueue->UpdateLastOutSamplePts(100);
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");;
    bool ret = true;
    ret = sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopConsume
 * @tc.number: 005
 * @tc.desc  : Test function ShouldStopConsume
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopConsume_05, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->UpdateLastEnterSamplePts(100);
    sampleQueue->UpdateLastOutSamplePts(200);
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");;
    bool ret = true;
    ret = sampleQueueController_->ShouldStopConsume(trackId, sampleQueue, task);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : TEST_ShouldStartProduce
 * @tc.number: 001
 * @tc.desc  : Test function ShouldStartProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStartProduce_01, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    std::unique_ptr<Task> task = nullptr;
    bool ret = true;
    ret = sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStartProduce
 * @tc.number: 002
 * @tc.desc  : Test function ShouldStartProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStartProduce_02, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = nullptr;
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStartProduce
 * @tc.number: 003
 * @tc.desc  : Test function ShouldStartProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStartProduce_03, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = nullptr;
    std::unique_ptr<Task> task = nullptr;
    bool ret = true;
    ret = sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStartProduce
 * @tc.number: 004
 * @tc.desc  : Test function ShouldStartProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStartProduce_04, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->UpdateLastEnterSamplePts(7 * 1000 * 1000);
    sampleQueue->UpdateLastOutSamplePts(1000 * 1000);
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStartProduce
 * @tc.number: 005
 * @tc.desc  : Test function ShouldStartProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStartProduce_05, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->UpdateLastEnterSamplePts(7 * 1000 * 1000);
    sampleQueue->UpdateLastOutSamplePts(2 * 1000 * 1000);
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStartProduce(trackId, sampleQueue, task);
    EXPECT_EQ(true, ret);
}

/**
 * @tc.name  : TEST_ShouldStopProduce
 * @tc.number: 001
 * @tc.desc  : Test function ShouldStopProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopProduce_01, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = nullptr;
    std::unique_ptr<Task> task = nullptr;
    bool ret = true;
    ret = sampleQueueController_->ShouldStopProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopProduce
 * @tc.number: 002
 * @tc.desc  : Test function ShouldStopProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopProduce_02, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = nullptr;
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStopProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopProduce
 * @tc.number: 003
 * @tc.desc  : Test function ShouldStopProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopProduce_03, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    std::unique_ptr<Task> task = nullptr;
    bool ret = true;
    ret = sampleQueueController_->ShouldStopProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopProduce
 * @tc.number: 004
 * @tc.desc  : Test function ShouldStopProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopProduce_04, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    SampleQueue::Config config = {};
    config.queueSize_ = 10;
    sampleQueue->Init(config);
    sampleQueue->UpdateLastEnterSamplePts(7 * 1000 * 1000);
    sampleQueue->UpdateLastOutSamplePts(2 * 1000 * 1000);
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStopProduce(trackId, sampleQueue, task);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name  : TEST_ShouldStopProduce
 * @tc.number: 005
 * @tc.desc  : Test function ShouldStopProduce
 */
HWTEST_F(SampleQueueControllerUnitTest, TEST_ShouldStopProduce_05, TestSize.Level1)
{
    int32_t trackId = 1;
    std::shared_ptr<SampleQueue> sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->UpdateLastEnterSamplePts(13 * 1000 * 1000);
    sampleQueue->UpdateLastOutSamplePts(2 * 1000 * 1000);
    std::unique_ptr<Task> task = std::make_unique<Task>("wz");
    bool ret = true;
    ret = sampleQueueController_->ShouldStopProduce(trackId, sampleQueue, task);
    EXPECT_EQ(true, ret);
}
}
}  // namespace OHOS::Media