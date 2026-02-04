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

#include <ctime>
#include <sys/time.h>
#include "sync_fence.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "metadata_filter.h"

#include "metadata_filter_unit_test.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
using namespace testing;
void MetaDataFilterUnitTest::SetUpTestCase(void)
{
}

void MetaDataFilterUnitTest::TearDownTestCase(void)
{
}

void MetaDataFilterUnitTest::SetUp()
{
    metaData_ = std::make_shared<Pipeline::MetaDataFilter>("testMetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
}

void MetaDataFilterUnitTest::TearDown()
{
    metaData_ = nullptr;
}

HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    metaData->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> format;
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	    AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    metaData->OnLinkedResult(inputBufferQueueProducer, format);

    EXPECT_EQ(metaData->SetCodecFormat(format), Status::OK);
    EXPECT_EQ(metaData->Configure(format), Status::OK);
    EXPECT_EQ(metaData->SetInputMetaSurface(nullptr), Status::ERROR_UNKNOWN);
    EXPECT_EQ(metaData->SetInputMetaSurface(metaData->GetInputMetaSurface()), Status::OK);
    EXPECT_EQ(metaData->DoPrepare(), Status::OK);
    EXPECT_EQ(metaData->DoStart(), Status::OK);
    metaData->OnBufferAvailable();
    EXPECT_EQ(metaData->DoPause(), Status::OK);
    EXPECT_EQ(metaData->DoResume(), Status::OK);
    EXPECT_EQ(metaData->DoFlush(), Status::OK);
    EXPECT_EQ(metaData->DoStop(), Status::OK);
    metaData->OnBufferAvailable();
    EXPECT_EQ(metaData->DoRelease(), Status::OK);
    EXPECT_EQ(metaData->NotifyEos(), Status::OK);
    metaData->SetParameter(format);
    metaData->GetParameter(format);

    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    EXPECT_EQ(metaData->LinkNext(nextFilter, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(metaData->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(metaData->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    metaData->GetFilterType();

    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->pts_ = 0;
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 70;

    metaData->UpdateBufferConfig(buffer, 50);
    metaData->UpdateBufferConfig(buffer, 100);

    metaData->OnUpdatedResult(format);
    metaData->OnUnlinkedResult(format);

    EXPECT_EQ(metaData->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, nullptr), Status::OK);
    EXPECT_EQ(metaData->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, nullptr), Status::OK);
    EXPECT_EQ(metaData->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, nullptr), Status::OK);
}

HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    sptr<SurfaceBuffer> buffer = avbuffer->memory_->GetSurfaceBuffer();
    metaData->inputSurface_ = Surface::CreateSurfaceAsConsumer("MetadataSurface");
    metaData->outputBufferQueueProducer_ = new MyAVBufferQueueProducer();
    metaData->isStop_ = true;
    metaData->OnBufferAvailable();
    metaData->isStop_ = false;
    metaData->OnBufferAvailable();
    EXPECT_EQ(metaData->totalPausedTime_, 0);
}

HWTEST_F(MetaDataFilterUnitTest, UpdateBufferConfig_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->refreshTotalPauseTime_ = false;
    metaData->UpdateBufferConfig(avbuffer, 0);
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 0;
    metaData->latestBufferTime_  = 0;
    metaData->UpdateBufferConfig(avbuffer, 0);
    metaData->latestPausedTime_ = 0;
    metaData->latestBufferTime_  = -1;
    metaData->UpdateBufferConfig(avbuffer, 0);
    metaData->latestPausedTime_ = 0;
    metaData->latestBufferTime_  = 1;
    metaData->UpdateBufferConfig(avbuffer, 0);
    EXPECT_EQ(metaData->totalPausedTime_, 0);
}

sptr<MockConsumerSurface> MockConsumerSurface::CreateSurfaceAsConsumer(std::string name)
{
    sptr<MockConsumerSurface> surf = new MockConsumerSurface(name);
    sptr<BufferQueue> queue_ = new BufferQueue(surf->name_);
    surf->producer_ = new BufferQueueProducer(queue_);
    surf->consumer_ = new BufferQueueConsumer(queue_);
    surf->uniqueId_ = surf->producer_->GetUniqueId();
    return surf;
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_001
 * @tc.desc: ret == GSERROR_OK && buffer != nullptr, then isStop_ = true
 * @tc.type FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_001, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    metaData_->isStop_ = true;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_002
 * @tc.desc: if extraData exist, timestamp <= latestBufferTime_
 * @tc.type FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_002, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    metaData_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)-2);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_003
 * @tc.desc: if extraData exist, timestamp = 0
 * @tc.type FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_003, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    metaData_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)0);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_004
 * @tc.desc: if status != OK
 * @tc.type FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_004, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    metaData_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));

    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::ERROR_NULL_POINTER)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_005
 * @tc.desc: if status == OK, emptyOutputBuffer->memory_ == nullptr
 * @tc.type FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_005, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    metaData_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));

    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    mockEmptyOutputBuffer->memory_ = nullptr;
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_006
 * @tc.desc: if status == OK, emptyOutputBuffer->memory_ != nullptr
 * @tc.type FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_006, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    metaData_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));

    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    mockEmptyOutputBuffer->memory_ = std::make_shared<AVMemory>();
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_)).Times(1);
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}
}