/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "video_capture_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace Media {
namespace Pipeline {
void VideoCaptureFilterUnitTest::SetUpTestCase(void) {}

void VideoCaptureFilterUnitTest::TearDownTestCase(void) {}

void VideoCaptureFilterUnitTest::SetUp(void)
{
    videoCaptureFilter_ = std::make_shared<VideoCaptureFilter>("testVideoCaptureFilter", FilterType::VIDEO_CAPTURE);
}

void VideoCaptureFilterUnitTest::TearDown(void)
{
    videoCaptureFilter_ = nullptr;
}

/**
 * @tc.name: VideoCaptureFilter_Init_001
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_Init_001, TestSize.Level1)
{
    std::shared_ptr<TestEventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> testFilterCallback = std::make_shared<TestFilterCallback>();
    videoCaptureFilter_->Init(testEventReceiver, testFilterCallback);

    EXPECT_EQ(videoCaptureFilter_->eventReceiver_, testEventReceiver);
}

/**
 * @tc.name: VideoCaptureFilter_Configure_001
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_Configure_001, TestSize.Level1)
{
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    Status ret = videoCaptureFilter_->Configure(videoMeta);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_SetInputSurface_001
 * @tc.desc: SetInputSurface
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_SetInputSurface_001, TestSize.Level1)
{
    sptr<Surface> consumerSurface = nullptr;
    Status ret = videoCaptureFilter_->SetInputSurface(consumerSurface);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    consumerSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_TRUE(consumerSurface);
    ret = videoCaptureFilter_->SetInputSurface(consumerSurface);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_GetInputSurface_001
 * @tc.desc: GetInputSurface
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_GetInputSurface_001, TestSize.Level1)
{
    sptr<Surface> producerSurface = videoCaptureFilter_->GetInputSurface();
    EXPECT_NE(producerSurface, nullptr);
    EXPECT_NE(producerSurface, videoCaptureFilter_->inputSurface_);
}

/**
 * @tc.name: VideoCaptureFilter_DoPrepare_001
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoPrepare_001, TestSize.Level1)
{
    videoCaptureFilter_->filterCallback_ = std::make_shared<TestFilterCallback>();
    Status ret = videoCaptureFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_DoStart_001
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoStart_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoCaptureFilter_->isStop_, false);
}

/**
 * @tc.name: VideoCaptureFilter_DoPause_001
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoPause_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoCaptureFilter_->isStop_, true);
}

/**
 * @tc.name: VideoCaptureFilter_DoResume_001
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoResume_001, TestSize.Level1)
{
    EXPECT_EQ(videoCaptureFilter_->refreshTotalPauseTime_, false);
    Status ret = videoCaptureFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoCaptureFilter_->refreshTotalPauseTime_, true);
}

/**
 * @tc.name: VideoCaptureFilter_DoStop_001
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoStop_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->DoStop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoCaptureFilter_->latestBufferTime_, TIME_NONE);
    EXPECT_EQ(videoCaptureFilter_->latestPausedTime_, TIME_NONE);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 0);
    EXPECT_EQ(videoCaptureFilter_->refreshTotalPauseTime_, false);
}

/**
 * @tc.name: VideoCaptureFilter_DoFlush_001
 * @tc.desc: DoFlush
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoFlush_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->DoFlush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_DoRelease_001
 * @tc.desc: DoRelease
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoRelease_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_NotifyEos_001
 * @tc.desc: NotifyEos
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_NotifyEos_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->NotifyEos();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_LinkNext_001
 * @tc.desc: LinkNext
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_LinkNext_001, TestSize.Level1)
{
    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    StreamType outType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status ret = videoCaptureFilter_->LinkNext(nextFilter, outType);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateNext_001
 * @tc.desc: UpdateNext
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateNext_001, TestSize.Level1)
{
    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    StreamType outType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status ret = videoCaptureFilter_->UpdateNext(nextFilter, outType);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_UnLinkNext_001
 * @tc.desc: UnLinkNext
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UnLinkNext_001, TestSize.Level1)
{
    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    StreamType outType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status ret = videoCaptureFilter_->UnLinkNext(nextFilter, outType);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_GetFilterType_001
 * @tc.desc: GetFilterType
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_GetFilterType_001, TestSize.Level1)
{
    videoCaptureFilter_->filterType_ = FilterType::VIDEO_CAPTURE;
    FilterType filterType_ = videoCaptureFilter_->GetFilterType();
    EXPECT_EQ(filterType_, FilterType::VIDEO_CAPTURE);
}

/**
 * @tc.name: VideoCaptureFilter_OnLinked_001
 * @tc.desc: OnLinked
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnLinked_001, TestSize.Level1)
{
    std::shared_ptr<FilterLinkCallback> testFilterLinkCallback = std::make_shared<TestFilterLinkCallback>();
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    StreamType inType = Pipeline::StreamType::STREAMTYPE_PACKED;
    Status ret = videoCaptureFilter_->OnLinked(inType, videoMeta, testFilterLinkCallback);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_OnUpdated_001
 * @tc.desc: OnUpdated
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnUpdated_001, TestSize.Level1)
{
    std::shared_ptr<FilterLinkCallback> testFilterLinkCallback = std::make_shared<TestFilterLinkCallback>();
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    StreamType inType = Pipeline::StreamType::STREAMTYPE_PACKED;
    Status ret = videoCaptureFilter_->OnUpdated(inType, videoMeta, testFilterLinkCallback);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_OnUnLinked_001
 * @tc.desc: OnUnLinked
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnUnLinked_001, TestSize.Level1)
{
    std::shared_ptr<FilterLinkCallback> testFilterLinkCallback = std::make_shared<TestFilterLinkCallback>();
    StreamType inType = Pipeline::StreamType::STREAMTYPE_PACKED;
    Status ret = videoCaptureFilter_->OnUnLinked(inType, testFilterLinkCallback);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_OnLinkedResult_001
 * @tc.desc: OnLinkedResult
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnLinkedResult_001, TestSize.Level1)
{
    sptr<AVBufferQueueProducer> testOutputBufferQueue = new OHOS::Media::Pipeline::MockAVBufferQueueProducer();
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoCaptureFilter_->OnLinkedResult(testOutputBufferQueue, videoMeta);
    EXPECT_EQ(videoCaptureFilter_->outputBufferQueueProducer_, testOutputBufferQueue);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_001
 * @tc.desc: UpdateBufferConfig
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_001, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = TIME_NONE;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    uint32_t flag = (uint32_t)Plugins::AVBufferFlag::SYNC_FRAME | (uint32_t)Plugins::AVBufferFlag::CODEC_DATA;
    EXPECT_EQ(buffer->flag_, flag);
    EXPECT_EQ(videoCaptureFilter_->startBufferTime_, 100);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_002
 * @tc.desc: UpdateBufferConfig
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_002, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 100;
    videoCaptureFilter_->refreshTotalPauseTime_ = false;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(buffer->pts_, (100 - 100 - 0) / 1000);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_003
 * @tc.desc: UpdateBufferConfig
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_003, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 100;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(buffer->pts_, (100 - 100 - 0) / 1000);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_004
 * @tc.desc: UpdateBufferConfig
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_004, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 50;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = 90;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 100 - 90);
    EXPECT_EQ(buffer->pts_, (100 - 50 - 10) / 1000);
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
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_001
 * @tc.desc: isStop_ = true, buffer should be released
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_001, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    EXPECT_NE(mockBuffer, nullptr);
    videoCaptureFilter_->isStop_ = true;
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence), Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->isStop_, true);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_002
 * @tc.desc: AcquireBuffer returns OK but buffer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_002, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    videoCaptureFilter_->isStop_ = false;
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(Return(OHOS::GSError::GSERROR_OK));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(0);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->isStop_, false);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_003
 * @tc.desc: AcquireBuffer returns error
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_003, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), Return(OHOS::GSError::GSERROR_NO_PERMISSION)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(0);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_004
 * @tc.desc: bufferSize <= 0 should be rejected (dataSize not set, defaults to 0)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_004, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    videoCaptureFilter_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    // No dataSize set in extraData, bufferSize defaults to 0
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->isStop_, false);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_005
 * @tc.desc: bufferSize > buffer->GetSize() should be rejected (OOB read prevention)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_005, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    videoCaptureFilter_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    // SurfaceBuffer::Create() has GetSize()=0, dataSize=65536 > 0 triggers OOB check
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)65536);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->isStop_, false);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_006
 * @tc.desc: negative dataSize should be rejected
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_006, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    videoCaptureFilter_->isStop_ = false;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    sptr<SyncFence> mockFence = new SyncFence(-1);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)-1);
    EXPECT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->isStop_, false);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_007
 * @tc.desc: bufferSize validation passes, full happy path through OnBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_007, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

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
    ASSERT_EQ(bufferSize, 256u);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)bufferSize);
    mockBuffer->GetExtraData()->ExtraSet("isKeyFrame", (int32_t)0);

    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    // Set up output buffer queue producer mock
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    uint8_t outputData[1024];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(Return(Status::OK));

    videoCaptureFilter_->isStop_ = false;
    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->startBufferTime_, (int64_t)1);
}

HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_SetCodecFormat_001, TestSize.Level1)
{
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    Status ret = videoCaptureFilter_->SetCodecFormat(videoMeta);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_001
 * @tc.desc: RequestBuffer returns error, ProcessAndPushOutputBuffer should fail
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_001, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::ERROR_UNKNOWN)));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_002
 * @tc.desc: RequestBuffer OK but memory_ is nullptr, ProcessAndPushOutputBuffer should fail
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_002, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = std::make_shared<AVBuffer>();
    mockEmptyOutputBuffer->memory_ = nullptr;
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_003
 * @tc.desc: RequestBuffer OK, memory_ valid, PushBuffer success
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_003, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    // Create output buffer with real allocated memory
    uint8_t outputData[64];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    ASSERT_NE(mockEmptyOutputBuffer, nullptr);
    ASSERT_NE(mockEmptyOutputBuffer->memory_, nullptr);
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(Return(Status::OK));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 1;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_TRUE(result);
    // UpdateBufferConfig overwrites flag to SYNC_FRAME | CODEC_DATA when startBufferTime_ == TIME_NONE
    EXPECT_EQ(mockEmptyOutputBuffer->flag_,
              static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME) |
              static_cast<uint32_t>(Plugins::AVBufferFlag::CODEC_DATA));
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_012
 * @tc.desc: UpdateBufferConfig with latestPausedTime_ == TIME_NONE, skip pause accumulation
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_012, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 0;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = TIME_NONE;
    videoCaptureFilter_->totalPausedTime_ = 50;
    // latestPausedTime_ == TIME_NONE, skip accumulation
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 50);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_005
 * @tc.desc: UpdateBufferConfig with totalPausedTime_ overflow, should skip accumulation
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_005, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 50;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = 90;
    // Set totalPausedTime_ near INT64_MAX to trigger overflow
    videoCaptureFilter_->totalPausedTime_ = INT64_MAX - 5;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    // timeDiff = 100 - 90 = 10, totalPausedTime_ + 10 overflows, skip accumulation
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, INT64_MAX - 5);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_006
 * @tc.desc: UpdateBufferConfig with normal totalPausedTime_, no overflow
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_006, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 50;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = 90;
    videoCaptureFilter_->totalPausedTime_ = 100;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 200);
    // pauseDuration = 200 - 90 = 110, totalPausedTime_ = 100 + 110 = 210, no overflow
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 210);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_007
 * @tc.desc: UpdateBufferConfig with elapsed <= totalPausedTime_, pts should be 0
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_007, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 100;
    videoCaptureFilter_->refreshTotalPauseTime_ = false;
    videoCaptureFilter_->totalPausedTime_ = 200;
    // elapsed = 150 - 100 = 50, 50 <= 200, pts = 0
    videoCaptureFilter_->UpdateBufferConfig(buffer, 150);
    EXPECT_EQ(buffer->pts_, 0);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_008
 * @tc.desc: UpdateBufferConfig with latestBufferTime_ <= latestPausedTime_, skip accumulation
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_008, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 0;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = 200;
    videoCaptureFilter_->totalPausedTime_ = 50;
    // latestBufferTime_(100) <= latestPausedTime_(200), skip accumulation
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 50);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_009
 * @tc.desc: UpdateBufferConfig with timeDiff subtraction overflow, skip accumulation
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_009, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 0;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = INT64_MIN;
    videoCaptureFilter_->totalPausedTime_ = 50;
    // latestPausedTime_ < 0 && latestBufferTime_ > INT64_MAX + INT64_MIN, timeDiff overflow
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 50);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_010
 * @tc.desc: UpdateBufferConfig with startBufferTime_ < 0, elapsed = 0 (IPC defense)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_010, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = -1;
    videoCaptureFilter_->refreshTotalPauseTime_ = false;
    videoCaptureFilter_->totalPausedTime_ = 0;
    // startBufferTime_ < 0, elapsed = 0, pts = 0
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(buffer->pts_, 0);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_011
 * @tc.desc: UpdateBufferConfig with timestamp < startBufferTime_, elapsed = 0 (IPC defense)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_011, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 200;
    videoCaptureFilter_->refreshTotalPauseTime_ = false;
    videoCaptureFilter_->totalPausedTime_ = 0;
    // timestamp(50) < startBufferTime_(200), elapsed = 0, pts = 0
    videoCaptureFilter_->UpdateBufferConfig(buffer, 50);
    EXPECT_EQ(buffer->pts_, 0);
}

/**
 * @tc.name: VideoCaptureFilter_DoPrepare_002
 * @tc.desc: DoPrepare with null filterCallback_, should return ERROR_NULL_POINTER
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_DoPrepare_002, TestSize.Level1)
{
    EXPECT_EQ(videoCaptureFilter_->filterCallback_, nullptr);
    Status ret = videoCaptureFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: VideoCaptureFilter_AcquireInputBuffer_001
 * @tc.desc: AcquireInputBuffer with inputSurface_ == nullptr, should return false
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_AcquireInputBuffer_001, TestSize.Level1)
{
    EXPECT_EQ(videoCaptureFilter_->inputSurface_, nullptr);
    sptr<SurfaceBuffer> buffer;
    int64_t timestamp = 0;
    int32_t bufferSize = 0;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->AcquireInputBuffer(buffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_004
 * @tc.desc: ProcessAndPushOutputBuffer with outputBufferQueueProducer_ == nullptr, should return false
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_004, TestSize.Level1)
{
    EXPECT_EQ(videoCaptureFilter_->outputBufferQueueProducer_, nullptr);
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_005
 * @tc.desc: RequestBuffer returns OK but emptyOutputBuffer stays nullptr, should return false
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_005, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(Return(Status::OK));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_006
 * @tc.desc: PushBuffer returns error, ProcessAndPushOutputBuffer should return false
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_006, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    uint8_t outputData[64];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    ASSERT_NE(mockEmptyOutputBuffer, nullptr);
    ASSERT_NE(mockEmptyOutputBuffer->memory_, nullptr);
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    int64_t timestamp = 100;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_007
 * @tc.desc: isKeyFrame=0 with startBufferTime_ set, flag should be 0 (not overwritten by UpdateBufferConfig)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_007, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    uint8_t outputData[64];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    ASSERT_NE(mockEmptyOutputBuffer, nullptr);
    mockEmptyOutputBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    videoCaptureFilter_->startBufferTime_ = 100;
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(Return(Status::OK));

    int64_t timestamp = 200;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 0;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_TRUE(result);
    EXPECT_EQ(mockEmptyOutputBuffer->flag_, 0u);
}

/**
 * @tc.name: VideoCaptureFilter_ProcessAndPushOutputBuffer_008
 * @tc.desc: isKeyFrame=1 with startBufferTime_ set, flag should be SYNC_FRAME (not overwritten by UpdateBufferConfig)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ProcessAndPushOutputBuffer_008, TestSize.Level1)
{
    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    uint8_t outputData[64];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer = AVBuffer::CreateAVBuffer(outputData, sizeof(outputData),
                                                                                sizeof(outputData));
    ASSERT_NE(mockEmptyOutputBuffer, nullptr);
    mockEmptyOutputBuffer->flag_ = 0u;
    videoCaptureFilter_->startBufferTime_ = 100;
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(Return(Status::OK));

    int64_t timestamp = 200;
    int32_t bufferSize = 10;
    int32_t isKeyFrame = 1;
    bool result = videoCaptureFilter_->ProcessAndPushOutputBuffer(mockBuffer, timestamp, bufferSize, isKeyFrame);
    EXPECT_TRUE(result);
    EXPECT_EQ(mockEmptyOutputBuffer->flag_, static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME));
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_008
 * @tc.desc: AcquireInputBuffer succeeds but ProcessAndPushOutputBuffer fails (nullptr producer)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_008, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

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
    uint32_t bufferSize = mockBuffer->GetSize();
    ASSERT_EQ(bufferSize, 256u);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)1);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)bufferSize);
    mockBuffer->GetExtraData()->ExtraSet("isKeyFrame", (int32_t)0);

    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    videoCaptureFilter_->outputBufferQueueProducer_ = nullptr;
    videoCaptureFilter_->isStop_ = false;
    videoCaptureFilter_->startBufferTime_ = TIME_NONE;
    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->startBufferTime_, TIME_NONE);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_009
 * @tc.desc: AcquireBuffer returns OK with buffer but fence is null (L353 branch 4)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_009, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);

    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    videoCaptureFilter_->isStop_ = false;
    videoCaptureFilter_->startBufferTime_ = TIME_NONE;
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(0);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->startBufferTime_, TIME_NONE);
}

/**
 * @tc.name: VideoCaptureFilter_OnBufferAvailable_010
 * @tc.desc: buffer GetExtraData returns null, extraData false branch (L364)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_010, TestSize.Level1)
{
    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);

    sptr<SurfaceBuffer> mockBuffer = SurfaceBuffer::Create();
    ASSERT_NE(mockBuffer, nullptr);
    mockBuffer->SetExtraData(nullptr);
    sptr<SyncFence> mockFence = new SyncFence(-1);
    videoCaptureFilter_->isStop_ = false;
    videoCaptureFilter_->startBufferTime_ = TIME_NONE;
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    videoCaptureFilter_->OnBufferAvailable();
    EXPECT_EQ(videoCaptureFilter_->startBufferTime_, TIME_NONE);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_013
 * @tc.desc: latestPausedTime_ < 0 but no timeDiff overflow, accumulation proceeds (L410 branch 2)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_013, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 0;
    videoCaptureFilter_->refreshTotalPauseTime_ = true;
    videoCaptureFilter_->latestPausedTime_ = -10;
    videoCaptureFilter_->totalPausedTime_ = 0;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 100);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 110);
}

/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_014
 * @tc.desc: ternary true branch with elapsed > totalPausedTime_ producing pts_ > 0 (L435 branch 1)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_014, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    videoCaptureFilter_->startBufferTime_ = 0;
    videoCaptureFilter_->refreshTotalPauseTime_ = false;
    videoCaptureFilter_->totalPausedTime_ = 0;
    videoCaptureFilter_->UpdateBufferConfig(buffer, 1000000);
    EXPECT_EQ(buffer->pts_, 1000);
}

/**
 * @tc.name: VideoCaptureFilter_ConsumerSurfaceBufferListener_001
 * @tc.desc: ConsumerSurfaceBufferListener forwards OnBufferAvailable to filter (L87 true branch)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ConsumerSurfaceBufferListener_001, TestSize.Level1)
{
    sptr<Surface> producerSurface = videoCaptureFilter_->GetInputSurface();
    ASSERT_NE(producerSurface, nullptr);

    sptr<Surface> realSurface = videoCaptureFilter_->inputSurface_;
    ConsumerSurface* cs = static_cast<ConsumerSurface*>(realSurface.GetRefPtr());
    ASSERT_NE(cs, nullptr);
    ASSERT_NE(cs->consumer_, nullptr);
    ASSERT_NE(cs->consumer_->bufferQueue_, nullptr);
    sptr<IBufferConsumerListener> listener = cs->consumer_->bufferQueue_->listener_;
    ASSERT_NE(listener, nullptr);

    sptr<MockConsumerSurface> mockConsumerSurface =
        MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    videoCaptureFilter_->inputSurface_ = mockConsumerSurface;

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
    uint32_t bufferSize = mockBuffer->GetSize();
    ASSERT_EQ(bufferSize, 256u);
    mockBuffer->GetExtraData()->ExtraSet("timeStamp", (int64_t)42);
    mockBuffer->GetExtraData()->ExtraSet("dataSize", (int32_t)bufferSize);
    mockBuffer->GetExtraData()->ExtraSet("isKeyFrame", (int32_t)0);

    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockBuffer), SetArgReferee<1>(mockFence),
                        Return(OHOS::GSError::GSERROR_OK)));
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(1);

    sptr<MockAVBufferQueueProducer> mockAVBufferQueueProducer = new MockAVBufferQueueProducer();
    videoCaptureFilter_->outputBufferQueueProducer_ = mockAVBufferQueueProducer;
    uint8_t outputData[1024];
    std::shared_ptr<AVBuffer> mockEmptyOutputBuffer =
        AVBuffer::CreateAVBuffer(outputData, sizeof(outputData), sizeof(outputData));
    EXPECT_CALL(*mockAVBufferQueueProducer, RequestBuffer(testing::_, testing::_, testing::_))
        .WillOnce(DoAll(SetArgReferee<0>(mockEmptyOutputBuffer), Return(Status::OK)));
    EXPECT_CALL(*mockAVBufferQueueProducer, PushBuffer(testing::_, testing::_))
        .WillOnce(Return(Status::OK));

    videoCaptureFilter_->isStop_ = false;
    videoCaptureFilter_->startBufferTime_ = TIME_NONE;

    listener->OnBufferAvailable();

    EXPECT_EQ(videoCaptureFilter_->startBufferTime_, (int64_t)42);
}

/**
 * @tc.name: VideoCaptureFilter_ConsumerSurfaceBufferListener_002
 * @tc.desc: ConsumerSurfaceBufferListener lock fails after filter destroyed (L87 false branch)
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_ConsumerSurfaceBufferListener_002, TestSize.Level1)
{
    std::shared_ptr<VideoCaptureFilter> tempFilter =
        std::make_shared<VideoCaptureFilter>("temp", FilterType::VIDEO_CAPTURE);
    sptr<Surface> producerSurface = tempFilter->GetInputSurface();
    ASSERT_NE(producerSurface, nullptr);

    sptr<Surface> realSurface = tempFilter->inputSurface_;
    ConsumerSurface* cs = static_cast<ConsumerSurface*>(realSurface.GetRefPtr());
    ASSERT_NE(cs, nullptr);
    ASSERT_NE(cs->consumer_, nullptr);
    ASSERT_NE(cs->consumer_->bufferQueue_, nullptr);
    sptr<IBufferConsumerListener> listener = cs->consumer_->bufferQueue_->listener_;
    ASSERT_NE(listener, nullptr);

    sptr<MockConsumerSurface> mockConsumerSurface =
        MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    tempFilter->inputSurface_ = mockConsumerSurface;
    EXPECT_CALL(*mockConsumerSurface, AcquireBuffer(testing::_, testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(*mockConsumerSurface, ReleaseBuffer(testing::_, testing::_)).Times(0);

    tempFilter.reset();

    listener->OnBufferAvailable();

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(mockConsumerSurface.GetRefPtr()));
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
