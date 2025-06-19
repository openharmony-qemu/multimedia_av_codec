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

#include "sei_parser_filter_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int32_t NUM_1 = 1;

void SeiParserFilterUnitTest::SetUpTestCase(void) {}

void SeiParserFilterUnitTest::TearDownTestCase(void) {}

void SeiParserFilterUnitTest::SetUp(void)
{
    seiParserFilter_ = std::make_shared<SeiParserFilter>("testSeiParserFilter", FilterType::FILTERTYPE_SEI);
}

void SeiParserFilterUnitTest::TearDown(void)
{
    seiParserFilter_ = nullptr;
}

/**
 * @tc.name  : Test PrepareState
 * @tc.number: PrepareState_001
 * @tc.desc  : Test ret != Status::OK
 */
HWTEST_F(SeiParserFilterUnitTest, PrepareState_001, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    auto inputBufferQueue = std::make_shared<MockAVBufferQueue>();
    seiParserFilter_->inputBufferQueue_ = inputBufferQueue;
    EXPECT_CALL(*inputBufferQueue, GetQueueSize()).WillOnce(Return(NUM_1));
    auto ret = seiParserFilter_->PrepareState();
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(seiParserFilter_->state_, Pipeline::FilterState::INITIALIZED);
}

/**
 * @tc.name  : Test PrepareInputBufferQueue
 * @tc.number: PrepareInputBufferQueue_001
 * @tc.desc  : Test inputBufferQueue_ != nullptr && inputBufferQueue_->GetQueueSize() > 0
 */
HWTEST_F(SeiParserFilterUnitTest, PrepareInputBufferQueue_001, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    auto inputBufferQueue = std::make_shared<MockAVBufferQueue>();
    seiParserFilter_->inputBufferQueue_ = inputBufferQueue;
    EXPECT_CALL(*inputBufferQueue, GetQueueSize()).WillOnce(Return(NUM_1));
    auto ret = seiParserFilter_->PrepareInputBufferQueue();
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name  : Test GetBufferQueueProducer
 * @tc.number: GetBufferQueueProducer_001
 * @tc.desc  : Test state_ != Pipeline::FilterState::READY
 */
HWTEST_F(SeiParserFilterUnitTest, GetBufferQueueProducer_001, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->state_ = Pipeline::FilterState::INITIALIZED;
    auto ret = seiParserFilter_->GetBufferQueueProducer();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test GetBufferQueueProducer
 * @tc.number: GetBufferQueueProducer_002
 * @tc.desc  : Test state_ == Pipeline::FilterState::READY
 */
HWTEST_F(SeiParserFilterUnitTest, GetBufferQueueProducer_002, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->state_ = Pipeline::FilterState::READY;
    seiParserFilter_->inputBufferQueueProducer_ = new MockAVBufferQueueProducer();
    auto ret = seiParserFilter_->GetBufferQueueProducer();
    EXPECT_NE(ret, nullptr);
    delete seiParserFilter_->inputBufferQueueProducer_;
}

/**
 * @tc.name  : Test GetBufferQueueConsumer
 * @tc.number: GetBufferQueueConsumer_001
 * @tc.desc  : Test state_ != Pipeline::FilterState::READY
 */
HWTEST_F(SeiParserFilterUnitTest, GetBufferQueueConsumer_001, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->state_ = Pipeline::FilterState::INITIALIZED;
    auto ret = seiParserFilter_->GetBufferQueueConsumer();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test GetBufferQueueConsumer
 * @tc.number: GetBufferQueueConsumer_002
 * @tc.desc  : Test state_ == Pipeline::FilterState::READY
 */
HWTEST_F(SeiParserFilterUnitTest, GetBufferQueueConsumer_002, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->state_ = Pipeline::FilterState::READY;
    seiParserFilter_->inputBufferQueueConsumer_ = new MockAVBufferQueueConsumer();
    auto ret = seiParserFilter_->GetBufferQueueConsumer();
    EXPECT_NE(ret, nullptr);
    delete seiParserFilter_->inputBufferQueueConsumer_;
}

/**
 * @tc.name  : Test SetSeiMessageCbStatus
 * @tc.number: SetSeiMessageCbStatus_001
 * @tc.desc  : Test producerListener_ == nullptr
 *             Test syncCenter_ != nullptr
 */
HWTEST_F(SeiParserFilterUnitTest, SetSeiMessageCbStatus_001, TestSize.Level0)
{
    bool status = true;
    std::vector<int32_t> payloadTypes = {};
    // Test producerListener_ == nullptr
    seiParserFilter_->producerListener_ = nullptr;
    seiParserFilter_->codecMimeType_ = "test";
    seiParserFilter_->inputBufferQueueProducer_ = new MockAVBufferQueueProducer();
    seiParserFilter_->eventReceiver_ = std::make_shared<MockEventReceiver>();
    // Test syncCenter_ != nullptr
    seiParserFilter_->syncCenter_ = std::make_shared<MockMediaSyncCenter>();
    auto ret = seiParserFilter_->SetSeiMessageCbStatus(status, payloadTypes);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SetSeiMessageCbStatus
 * @tc.number: SetSeiMessageCbStatus_002
 * @tc.desc  : Test syncCenter_ == nullptr
 */
HWTEST_F(SeiParserFilterUnitTest, SetSeiMessageCbStatus_002, TestSize.Level0)
{
    bool status = true;
    std::vector<int32_t> payloadTypes = {};
    seiParserFilter_->inputBufferQueueProducer_ = new MockAVBufferQueueProducer();
    seiParserFilter_->producerListener_ = nullptr;
    seiParserFilter_->codecMimeType_ = "test";
    seiParserFilter_->inputBufferQueueProducer_ = new MockAVBufferQueueProducer();;
    seiParserFilter_->eventReceiver_ = std::make_shared<MockEventReceiver>();
    seiParserFilter_->syncCenter_ = nullptr;
    auto ret = seiParserFilter_->SetSeiMessageCbStatus(status, payloadTypes);
    EXPECT_EQ(ret, Status::OK);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS