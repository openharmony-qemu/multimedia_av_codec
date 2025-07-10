/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "sample_queue_test.h"
#include "buffer/avbuffer_common.h"
#include "common/media_source.h"
#include "ibuffer_consumer_listener.h"
#include "graphic_common_c.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS::Media {
namespace Test {
static const int32_t NUM_TEST1 = 1;
static const uint32_t NUM_TEST2 = 2;
static const int64_t NUM_TEST3 = 3;
static const uint8_t NUM_TEST4 = 4;
static const int32_t NUM_0 = 0;
static const int32_t NUM_100 = 100;
static const int32_t NUM_200 = 200;
static const int32_t NUM_300 = 300;
static const int32_t NUM_1000 = 1000;
static const int32_t NUM_2000 = 2000;
static const int32_t NUM_4000 = 4000;
static const std::string STRING_TEST = "test";
static const float FLOAT_TEST3 = 3.0f;
static const double DOUBLE_TEST4 = 4.0;

void SampleQueueUnitTest::SetUpTestCase(void)
{}

void SampleQueueUnitTest::TearDownTestCase(void)
{}

void SampleQueueUnitTest::SetUp()
{
    sampleQueue_ = std::make_shared<SampleQueue>();
}

void SampleQueueUnitTest::TearDown()
{}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_001
 * @tc.desc  : Test case AnyValueType::INT32_T
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_001, TestSize.Level1)
{
    int32_t value = NUM_TEST1;
    auto meta = std::make_shared<Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = 1 | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_002
 * @tc.desc  : Test case AnyValueType::STRING
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_002, TestSize.Level1)
{
    std::string value = STRING_TEST;
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = test | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_003
 * @tc.desc  : Test case AnyValueType::BOOL
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_003, TestSize.Level1)
{
    bool value = true;
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = 1 | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_004
 * @tc.desc  : Test case AnyValueType::UINT32_T
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_004, TestSize.Level1)
{
    uint32_t value = NUM_TEST2;
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = 2 | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_005
 * @tc.desc  : Test case AnyValueType::INT64_T
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_005, TestSize.Level1)
{
    int64_t value = NUM_TEST3;
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = 3 | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_006
 * @tc.desc  : Test case AnyValueType::FLOAT
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_006, TestSize.Level1)
{
    float value = FLOAT_TEST3;
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = 3.000000 | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_007
 * @tc.desc  : Test case AnyValueType::DOUBLE
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_007, TestSize.Level1)
{
    double value = DOUBLE_TEST4;
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = 4.000000 | ");
}

/**
 * @tc.name  : Test StringifyMeta
 * @tc.number: StringifyMeta_008
 * @tc.desc  : Test default
 */
HWTEST_F(SampleQueueUnitTest, StringifyMeta_008, TestSize.Level1)
{
    std::vector<uint8_t> value;
    value.push_back(NUM_TEST4);
    auto meta = std::make_shared<Media::Meta>();
    EXPECT_TRUE(meta != nullptr);
    meta->SetData(Tag::REGULAR_TRACK_ID, value);
    std::string metaStr = sampleQueue_->StringifyMeta(meta);
    EXPECT_EQ(metaStr, "track_index = unknown type | ");
}

/**
 * @tc.name  : Test ResponseForSwitchDone
 * @tc.number: ResponseForSwitchDone_001
 * @tc.desc  : Test switchStatus_ == SelectBitrateStatus::SWITCHING
 */
HWTEST_F(SampleQueueUnitTest, ResponseForSwitchDone_001, TestSize.Level1)
{
    SampleQueue::Config sampleQueueConfig{};
    sampleQueueConfig.isFlvLiveStream_ = true;
    sampleQueueConfig.isSupportBitrateSwitch_ = true;
    sampleQueueConfig.queueId_ = NUM_TEST1;
    sampleQueueConfig.bufferCap_ = SampleQueue::MAX_SAMPLE_QUEUE_SIZE;
    Status status = sampleQueue_->Init(sampleQueueConfig);
    EXPECT_EQ(status, Status::OK);
    int64_t startPtsOnSwitch = NUM_TEST3;
    Status ret = sampleQueue_->DiscardSampleAfter(startPtsOnSwitch);
    EXPECT_EQ(ret, Status::OK);
    sampleQueue_->switchStatus_ = SelectBitrateStatus::SWITCHING;
    ret = sampleQueue_->ResponseForSwitchDone(startPtsOnSwitch);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(sampleQueue_->switchStatus_, SelectBitrateStatus::NORMAL);
}

/**
 * @tc.name  : Test ResponseForSwitchDone
 * @tc.number: ResponseForSwitchDone_002
 * @tc.desc  : Test switchStatus_ != SelectBitrateStatus::SWITCHING
 */
HWTEST_F(SampleQueueUnitTest, ResponseForSwitchDone_002, TestSize.Level1)
{
    SampleQueue::Config sampleQueueConfig{};
    sampleQueueConfig.isFlvLiveStream_ = true;
    sampleQueueConfig.isSupportBitrateSwitch_ = true;
    sampleQueueConfig.queueId_ = NUM_TEST1;
    sampleQueueConfig.bufferCap_ = SampleQueue::MAX_SAMPLE_QUEUE_SIZE;
    Status status = sampleQueue_->Init(sampleQueueConfig);
    EXPECT_EQ(status, Status::OK);
    int64_t startPtsOnSwitch = NUM_TEST3;
    Status ret = sampleQueue_->DiscardSampleAfter(startPtsOnSwitch);
    EXPECT_EQ(ret, Status::OK);
    sampleQueue_->switchStatus_ = SelectBitrateStatus::READY_SWITCH;
    ret = sampleQueue_->ResponseForSwitchDone(startPtsOnSwitch);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(sampleQueue_->switchStatus_, SelectBitrateStatus::READY_SWITCH);
}

/**
 * @tc.name  : Test CheckSwitchBitrateWaitList
 * @tc.number: CheckSwitchBitrateWaitList_001
 * @tc.desc  : Test it == switchBitrateWaitList_.end()
 */
HWTEST_F(SampleQueueUnitTest, CheckSwitchBitrateWaitList_001, TestSize.Level1)
{
    sampleQueue_->switchBitrateWaitList_.clear();

    uint32_t originalNextBitrate = sampleQueue_->nextSwitchBitrate_;
    SelectBitrateStatus originalStatus = sampleQueue_->switchStatus_;

    sampleQueue_->CheckSwitchBitrateWaitList();

    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, originalNextBitrate);
    EXPECT_EQ(sampleQueue_->switchStatus_, originalStatus);
    EXPECT_TRUE(sampleQueue_->switchBitrateWaitList_.empty());
}

/**
 * @tc.name  : Test CheckSwitchBitrateWaitList
 * @tc.number: CheckSwitchBitrateWaitList_002
 * @tc.desc  : Test it != switchBitrateWaitList_.end()
 */
HWTEST_F(SampleQueueUnitTest, CheckSwitchBitrateWaitList_002, TestSize.Level1)
{
    sampleQueue_->switchBitrateWaitList_.push_back(NUM_100);

    sampleQueue_->CheckSwitchBitrateWaitList();

    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, NUM_100);
    EXPECT_EQ(sampleQueue_->switchStatus_, SelectBitrateStatus::READY_SWITCH);
    EXPECT_TRUE(sampleQueue_->switchBitrateWaitList_.empty());
}

/**
 * @tc.name  : Test CheckSwitchBitrateWaitList
 * @tc.number: CheckSwitchBitrateWaitList_003
 * @tc.desc  : Test *it != nextSwitchBitrate_
 */
HWTEST_F(SampleQueueUnitTest, CheckSwitchBitrateWaitList_003, TestSize.Level1)
{
    sampleQueue_->switchBitrateWaitList_.push_back(NUM_200);

    sampleQueue_->CheckSwitchBitrateWaitList();

    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, NUM_200);
    EXPECT_EQ(sampleQueue_->switchStatus_, SelectBitrateStatus::READY_SWITCH);
    EXPECT_TRUE(sampleQueue_->switchBitrateWaitList_.empty());
}

/**
 * @tc.name  : Test CheckSwitchBitrateWaitList
 * @tc.number: CheckSwitchBitrateWaitList_004
 * @tc.desc  : Test *it == nextSwitchBitrate_
 */
HWTEST_F(SampleQueueUnitTest, CheckSwitchBitrateWaitList_004, TestSize.Level1)
{
    sampleQueue_->switchBitrateWaitList_ = {NUM_100, NUM_200, NUM_300};

    sampleQueue_->CheckSwitchBitrateWaitList();

    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, NUM_100);
    EXPECT_EQ(sampleQueue_->switchStatus_, SelectBitrateStatus::READY_SWITCH);
}

/**
 * @tc.name  : Test IsSwitchBitrateOK
 * @tc.number: IsSwitchBitrateOK_001
 * @tc.desc  : Test !IsKeyFrameAvailable()
 */
HWTEST_F(SampleQueueUnitTest, IsSwitchBitrateOK_001, TestSize.Level1)
{
    sampleQueue_->switchStatus_ = SelectBitrateStatus::READY_SWITCH;
    sampleQueue_->lastEndSamplePts_ = NUM_1000;
    sampleQueue_->keyFramePtsSet_.clear();

    bool result = sampleQueue_->IsSwitchBitrateOK();

    EXPECT_TRUE(!result);
    EXPECT_TRUE(!(sampleQueue_->IsKeyFrameAvailable()));
}

/**
 * @tc.name  : Test ReadySwitchBitrate
 * @tc.number: ReadySwitchBitrate_001
 * @tc.desc  : Test switchStatus_ != SelectBitrateStatus::SWITCHING
 */
HWTEST_F(SampleQueueUnitTest, ReadySwitchBitrate_001, TestSize.Level1)
{
    sampleQueue_->config_.isSupportBitrateSwitch_ = true;
    sampleQueue_->switchStatus_ = static_cast<SelectBitrateStatus>(-1);
    sampleQueue_->keyFramePtsSet_.clear();
    Status ret = sampleQueue_->ReadySwitchBitrate(NUM_2000);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, NUM_0);
}

/**
 * @tc.name  : Test ReadySwitchBitrate
 * @tc.number: ReadySwitchBitrate_002
 * @tc.desc  : Test !IsSwitchBitrateOK()
 */
HWTEST_F(SampleQueueUnitTest, ReadySwitchBitrate_002, TestSize.Level1)
{
    sampleQueue_->config_.isSupportBitrateSwitch_ = true;
    sampleQueue_->switchStatus_ = SelectBitrateStatus::NORMAL;
    sampleQueue_->keyFramePtsSet_.clear();
    sampleQueue_->lastEndSamplePts_ = NUM_1000;
    Status ret = sampleQueue_->ReadySwitchBitrate(NUM_4000);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(sampleQueue_->switchStatus_, SelectBitrateStatus::READY_SWITCH);
    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, NUM_4000);
}

/**
 * @tc.name  : Test Clear
 * @tc.number: Clear_001
 * @tc.desc  : Test sampleBufferQueueProducer_ == nullptr
 */
HWTEST_F(SampleQueueUnitTest, Clear_001, TestSize.Level1)
{
    sampleQueue_->sampleBufferQueueProducer_ = nullptr;
    Status ret = sampleQueue_->Clear();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SetLargerQueueSize
 * @tc.number: SetLargerQueueSize_001
 * @tc.desc  : Test SetLargerQueueSize
 */
HWTEST_F(SampleQueueUnitTest, SetLargerQueueSize_001, TestSize.Level1)
{
    sampleQueue_->config_.queueSize_ = SampleQueue::MAX_SAMPLE_QUEUE_SIZE;
    Status ret = sampleQueue_->SetLargerQueueSize(SampleQueue::MAX_SAMPLE_QUEUE_SIZE_ON_MUTE);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(config_.queueSize_, SampleQueue::MAX_SAMPLE_QUEUE_SIZE_ON_MUTE);

    sampleQueue_->config_.queueSize_ = SampleQueue::MAX_SAMPLE_QUEUE_SIZE_ON_MUTE;
    ret = sampleQueue_->SetLargerQueueSize(SampleQueue::MAX_SAMPLE_QUEUE_SIZE);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(config_.queueSize_, SampleQueue::MAX_SAMPLE_QUEUE_SIZE_ON_MUTE);

    sampleQueue_->config_.queueSize_ = SampleQueue::MAX_SAMPLE_QUEUE_SIZE;
    ret = sampleQueue_->SetLargerQueueSize(AVBUFFER_QUEUE_MAX_QUEUE_SIZE_FOR_LARGER + NUM_TEST1);
    EXPECT_NE(ret, Status::OK);
    EXPECT_EQ(config_.queueSize_, SampleQueue::MAX_SAMPLE_QUEUE_SIZE);
}
}
}  // namespace OHOS::Media