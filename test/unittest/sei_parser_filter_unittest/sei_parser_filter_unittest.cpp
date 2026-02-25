/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
using namespace OHOS::Media::Pipeline;

static const int32_t NUM_1 = 1;
static const int32_t TRACK_NUMH = 1080;
static const int32_t TRACK_NUMW = 1920;

void SeiParserFilterUnitTest::SetUpTestCase(void) {}

void SeiParserFilterUnitTest::TearDownTestCase(void) {}

void SeiParserFilterUnitTest::SetUp(void)
{
    seiParserFilter_ = std::make_shared<SeiParserFilter>("testSeiParserFilter", FilterType::FILTERTYPE_SEI);
    producerListener_ = std::make_shared<SeiParserFilter>("producerListener", FilterType::FILTERTYPE_SEI);
}

void SeiParserFilterUnitTest::TearDown(void)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_ = nullptr;
    ASSERT_NE(producerListener_, nullptr);
    producerListener_ = nullptr;
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
 * @tc.name  : Test SetSeiMessageCbStatus
 * @tc.number: SetSeiMessageCbStatus_001
 * @tc.desc  : Test producerListener_ == nullptr
 *             Test syncCenter_ != nullptr
 */
HWTEST_F(SeiParserFilterUnitTest, SetSeiMessageCbStatus_001, TestSize.Level0)
{
    bool status = true;
    std::vector<int32_t> payloadTypes = {};
    seiParserFilter_->producerListener_ = nullptr;
    seiParserFilter_->codecMimeType_ = "test";
    seiParserFilter_->inputBufferQueueProducer_ = new MockAVBufferQueueProducer();
    seiParserFilter_->eventReceiver_ = std::make_shared<MockEventReceiver>();
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

/**
 * @tc.name  : Test OnBufferAvailable
 * @tc.number: OnBufferAvailable_001
 * @tc.desc  : Test all
 */
HWTEST_F(SeiParserFilterUnitTest, OnBufferAvailable_001, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    SeiParserFilter::AVBufferAvailableListener listener(seiParserFilter_);
    ASSERT_NO_FATAL_FAILURE({listener.OnBufferAvailable();});
    Status processStatus = seiParserFilter_->ProcessInputBuffer(0, 0);
    ASSERT_EQ(processStatus, Status::OK);
}

/**
 * @tc.name  : Test PrepareState
 * @tc.number: PrepareState_002
 * @tc.desc  : return ERROR_INVALID_OPERATION
 */
HWTEST_F(SeiParserFilterUnitTest, PrepareState_002, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    auto mockBufferQueue = std::make_shared<OHOS::Media::Pipeline::MockAVBufferQueue>();
    seiParserFilter_->inputBufferQueue_ = mockBufferQueue;
    EXPECT_CALL(*mockBufferQueue, GetQueueSize()).WillOnce(Return(1));
    seiParserFilter_->state_ = Pipeline::FilterState::INITIALIZED;
    Status ret = seiParserFilter_->PrepareState();
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(seiParserFilter_->state_, Pipeline::FilterState::INITIALIZED);
}

/**
 * @tc.name  : Test PrepareState
 * @tc.number: PrepareState_003
 * @tc.desc  : PrepareInputBufferQueue trackMeta_ = nullptr ,return ERROR_UNKNOWN
 */
HWTEST_F(SeiParserFilterUnitTest, PrepareState_003, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->inputBufferQueue_ = nullptr;
    seiParserFilter_->trackMeta_ = nullptr;
    seiParserFilter_->state_ = Pipeline::FilterState::INITIALIZED;
    Status ret = seiParserFilter_->PrepareState();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(seiParserFilter_->state_, Pipeline::FilterState::INITIALIZED);
}

/**
 * @tc.name  : Test PrepareState
 * @tc.number: PrepareState_004
 * @tc.desc  : PrepareInputBufferQueue return OK，eventReceiver_ = nullptr.
 */
HWTEST_F(SeiParserFilterUnitTest, PrepareState_004, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->inputBufferQueue_ = nullptr;
    seiParserFilter_->trackMeta_ = std::make_shared<Meta>();
    seiParserFilter_->trackMeta_->Set<Tag::VIDEO_HEIGHT>(TRACK_NUMH);
    seiParserFilter_->trackMeta_->Set<Tag::VIDEO_WIDTH>(TRACK_NUMW);
    seiParserFilter_->eventReceiver_ = nullptr;
    seiParserFilter_->state_ = Pipeline::FilterState::INITIALIZED;
    auto mockQueue = std::make_shared<MockAVBufferQueue>();
    ON_CALL(*mockQueue, GetQueueSize()).WillByDefault(Return(0));
    EXPECT_CALL(*mockQueue, GetProducer()).
        WillOnce(Return(sptr<AVBufferQueueProducer>(new MockAVBufferQueueProducer())));
    EXPECT_CALL(*mockQueue, GetConsumer()).
        WillOnce(Return(sptr<AVBufferQueueConsumer>(new MockAVBufferQueueConsumer())));
    seiParserFilter_->inputBufferQueue_ = mockQueue;
    Status ret = seiParserFilter_->PrepareState();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(seiParserFilter_->state_, Pipeline::FilterState::READY);
}

/**
 * @tc.name  : Test GetBufferQueueProducer
 * @tc.number: GetBufferQueueProducer_002
 * @tc.desc  : state_ = READY，return inputBufferQueueProducer_
 */
HWTEST_F(SeiParserFilterUnitTest, GetBufferQueueProducer_002, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->state_ = Pipeline::FilterState::READY;
    sptr<AVBufferQueueProducer> retProducer = seiParserFilter_->GetBufferQueueProducer();
    EXPECT_EQ(retProducer, seiParserFilter_->inputBufferQueueProducer_);
}

/**
 * @tc.name  : Test GetBufferQueueConsumer
 * @tc.number: GetBufferQueueConsumer_002
 * @tc.desc  : state_ = READY，return inputBufferQueueConsumer_
 */
HWTEST_F(SeiParserFilterUnitTest, GetBufferQueueConsumer_002, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    seiParserFilter_->state_ = Pipeline::FilterState::READY;
    sptr<AVBufferQueueConsumer> retConsumer = seiParserFilter_->GetBufferQueueConsumer();
    EXPECT_EQ(retConsumer, seiParserFilter_->inputBufferQueueConsumer_);
}

/**
 * @tc.name  : Test DrainOutputBuffer
 * @tc.number: DrainOutputBuffer_001
 * @tc.desc  : Test all
 */
HWTEST_F(SeiParserFilterUnitTest, DrainOutputBuffer_001, TestSize.Level0)
{
    ASSERT_NE(seiParserFilter_, nullptr);
    auto mockConsumer = sptr<MockAVBufferQueueConsumer>(new MockAVBufferQueueConsumer());
    seiParserFilter_->inputBufferQueueConsumer_ = mockConsumer;
    seiParserFilter_->filledOutputBuffer_ = std::make_shared<AVBuffer>();
    bool flushed = true;
    EXPECT_CALL(*mockConsumer, AcquireBuffer(Ref(seiParserFilter_->filledOutputBuffer_))).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockConsumer, ReleaseBuffer(Ref(seiParserFilter_->filledOutputBuffer_))).Times(1);
    seiParserFilter_->DrainOutputBuffer(flushed);
    EXPECT_EQ(seiParserFilter_->filledOutputBuffer_ != nullptr, true);
}

/**
 * @tc.name  : Test SetSyncCenter
 * @tc.number: SetSyncCenter_001
 * @tc.desc  : Test all
 */
HWTEST_F(SeiParserFilterUnitTest, SetSyncCenter_001, TestSize.Level0)
{
    ASSERT_NE(producerListener_, nullptr);
    auto mockSyncCenter = std::make_shared<MockMediaSyncCenter>();
    producerListener_->SetSyncCenter(mockSyncCenter);
    EXPECT_EQ(producerListener_->syncCenter_, mockSyncCenter);
}

/**
 * @tc.name  : Test OnInterrupted
 * @tc.number: OnInterrupted_001
 * @tc.desc  : Test all
 */
HWTEST_F(SeiParserFilterUnitTest, OnInterrupted_001, TestSize.Level0)
{
    ASSERT_NE(producerListener_, nullptr);
    bool interrupted = true;
    producerListener_->OnInterrupted(interrupted);
    EXPECT_EQ(producerListener_->syncCenter_ == nullptr, interrupted);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS