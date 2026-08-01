/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.desc: isStop_ = true, buffer should be released
 * @tc.type: FUNC
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
 * @tc.desc: timestamp <= latestBufferTime_ should be rejected
 * @tc.type: FUNC
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
 * @tc.desc: timestamp = 0 should be rejected
 * @tc.type: FUNC
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
 * @tc.desc: bufferSize <= 0 should be rejected (dataSize not set, defaults to 0)
 * @tc.type: FUNC
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
    // No dataSize set in extraData, bufferSize defaults to 0
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_005
 * @tc.desc: bufferSize > buffer->GetSize() should be rejected (OOB read prevention)
 * @tc.type: FUNC
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
    // SurfaceBuffer::Create() has GetSize()=0, dataSize=65536 > 0 triggers the OOB check
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)65536);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_006
 * @tc.desc: negative dataSize should be rejected
 * @tc.type: FUNC
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
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)-1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_ProcessAndPushOutputBuffer_001
 * @tc.desc: RequestBuffer returns error, ProcessAndPushOutputBuffer should fail
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_ProcessAndPushOutputBuffer_001, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::ERROR_NULL_POINTER)));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    bool result = metaData_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: MetaDataFilter_ProcessAndPushOutputBuffer_002
 * @tc.desc: RequestBuffer OK but memory_ is nullptr, ProcessAndPushOutputBuffer should fail
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_ProcessAndPushOutputBuffer_002, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    mockEmptyOutputBuffer->memory_ = nullptr;
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::OK)));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    bool result = metaData_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: MetaDataFilter_ProcessAndPushOutputBuffer_003
 * @tc.desc: RequestBuffer OK, memory_ valid, PushBuffer success
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_ProcessAndPushOutputBuffer_003, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    // Create output buffer with real allocated memory
    uint8_t outputData[64];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    ASSERT_NE(mockEmptyOutputBuffer, nullptr);
    ASSERT_NE(mockEmptyOutputBuffer->memory_, nullptr);
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(testing::Return(Status::OK));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    bool result = metaData_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MetaDataFilter_OnBufferAvailable_007
 * @tc.desc: bufferSize validation passes, full happy path through OnBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_OnBufferAvailable_007, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = metaData_->SetInputMetaSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(metaData_->inputSurface_, nullptr);

    // Create a SurfaceBuffer with a manually allocated BufferHandle so GetSize() > 0
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    BufferHandle* handle = new BufferHandle();
    memset_s(handle, sizeof(BufferHandle), 0, sizeof(BufferHandle));
    handle->fd = -1;
    handle->width = 64;
    handle->stride = 64 * 4;
    handle->height = 1;
    handle->size = 64 * 4;
    handle->format = GRAPHIC_PIXEL_FMT_RGBA_8888;
    handle->usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    handle->virAddr = malloc(handle->size);
    ASSERT_NE(handle->virAddr, nullptr);
    mockBuffer->SetBufferHandle(handle);

    sptr<SyncFence> mockFence = new SyncFence(-1);
    // Set valid extraData with dataSize <= GetSize() so bufferSize check passes
    uint32_t bufferSize = mockBuffer->GetSize();
    ASSERT_GT(bufferSize, 0u);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)bufferSize);

    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        testing::Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    // Set up output buffer queue producer mock
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    metaData_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    uint8_t outputData[1024];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), testing::Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(testing::Return(Status::OK));

    metaData_->isStop_ = false;
    metaData_->OnBufferAvailable();
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_002
 * @tc.desc: UpdateBufferConfig with totalPausedTime_ addition overflow, should skip accumulation
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = 0;
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 90;
    metaData->totalPausedTime_ = INT64_MAX - 5;
    // timeDiff = 100 - 90 = 10, totalPausedTime_ + 10 overflows, skip accumulation
    metaData->UpdateBufferConfig(avbuffer, 100);
    EXPECT_EQ(metaData->totalPausedTime_, INT64_MAX - 5);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_003
 * @tc.desc: UpdateBufferConfig with normal totalPausedTime_, no overflow
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_003, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 90;
    metaData->totalPausedTime_ = 100;
    // pauseDuration = 200 - 90 = 110, totalPausedTime_ = 100 + 110 = 210
    metaData->UpdateBufferConfig(avbuffer, 200);
    EXPECT_EQ(metaData->totalPausedTime_, 210);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_004
 * @tc.desc: UpdateBufferConfig with totalPausedTime_ at INT64_MAX and zero pauseDuration
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_004, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 100;
    metaData->totalPausedTime_ = INT64_MAX;
    // pauseDuration = 100 - 100 = 0, INT64_MAX - 0 = INT64_MAX, no overflow triggered
    metaData->UpdateBufferConfig(avbuffer, 100);
    EXPECT_EQ(metaData->totalPausedTime_, INT64_MAX);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_005
 * @tc.desc: UpdateBufferConfig with normal pause accumulation and pts calculation
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_005, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = 0;
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 100;
    metaData->totalPausedTime_ = 50;
    // timeDiff = 200 - 100 = 100, totalPausedTime_ = 50 + 100 = 150
    metaData->UpdateBufferConfig(avbuffer, 200);
    EXPECT_EQ(metaData->totalPausedTime_, 150);
    // elapsed = 200 - 0 = 200, 200 > 150, pts = 200 - 150 = 50
    EXPECT_EQ(avbuffer->pts_, 50);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_006
 * @tc.desc: UpdateBufferConfig with elapsed <= totalPausedTime_, pts should be 0
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_006, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = 100;
    metaData->refreshTotalPauseTime_ = false;
    metaData->totalPausedTime_ = 200;
    // elapsed = 150 - 100 = 50, 50 <= 200, pts = 0
    metaData->UpdateBufferConfig(avbuffer, 150);
    EXPECT_EQ(avbuffer->pts_, 0);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_010
 * @tc.desc: UpdateBufferConfig with latestPausedTime_ == TIME_NONE, skip pause accumulation
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_010, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = 0;
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = TIME_NONE;
    metaData->totalPausedTime_ = 50;
    // latestPausedTime_ == TIME_NONE, skip accumulation
    metaData->UpdateBufferConfig(avbuffer, 100);
    EXPECT_EQ(metaData->totalPausedTime_, 50);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_007
 * @tc.desc: UpdateBufferConfig with timeDiff subtraction overflow, skip accumulation
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_007, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = 0;
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = INT64_MIN;
    metaData->totalPausedTime_ = 50;
    // latestPausedTime_ < 0 && latestBufferTime_ > INT64_MAX + INT64_MIN, timeDiff overflow
    metaData->UpdateBufferConfig(avbuffer, 100);
    EXPECT_EQ(metaData->totalPausedTime_, 50);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_008
 * @tc.desc: UpdateBufferConfig with startBufferTime_ < 0, elapsed = 0 (IPC defense)
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_008, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = -1;
    metaData->refreshTotalPauseTime_ = false;
    metaData->totalPausedTime_ = 0;
    // startBufferTime_ < 0, elapsed = 0, pts = 0
    metaData->UpdateBufferConfig(avbuffer, 100);
    EXPECT_EQ(avbuffer->pts_, 0);
}

/**
 * @tc.name: MetaDataFilter_UpdateBufferConfig_009
 * @tc.desc: UpdateBufferConfig with timestamp < startBufferTime_, elapsed = 0 (IPC defense)
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_UpdateBufferConfig_009, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    metaData->startBufferTime_ = 200;
    metaData->refreshTotalPauseTime_ = false;
    metaData->totalPausedTime_ = 0;
    // timestamp(50) < startBufferTime_(200), elapsed = 0, pts = 0
    metaData->UpdateBufferConfig(avbuffer, 50);
    EXPECT_EQ(avbuffer->pts_, 0);
}
}
