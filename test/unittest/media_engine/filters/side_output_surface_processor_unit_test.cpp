/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "side_output_surface_processor_unit_test.h"
#include "gmock/gmock.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;

namespace {
constexpr uint32_t MAX_BUFFER_SIZE = 100 * 1024 * 1024; // 100MB
}

namespace OHOS {
namespace Media {
namespace Pipeline {

void SideOutputSurfaceProcessorUnitTest::SetUpTestCase(void) {}

void SideOutputSurfaceProcessorUnitTest::TearDownTestCase(void) {}

void SideOutputSurfaceProcessorUnitTest::SetUp(void)
{
    processor_ = std::make_shared<TestableSideOutputSurfaceProcessor>();
    EXPECT_NE(processor_, nullptr);
    callback_ = std::make_shared<MockPostProcessorCallback>();
    receiver_ = std::make_shared<MockEventReceiver>();
    mockSurface_ = new MockSurface();
    mockBuffer_ = new MockSurfaceBuffer();
}

void SideOutputSurfaceProcessorUnitTest::TearDown(void)
{
    if (processor_ != nullptr) {
        processor_->Release();
        processor_ = nullptr;
    }
    callback_ = nullptr;
    receiver_ = nullptr;
    mockSurface_ = nullptr;
    mockBuffer_ = nullptr;
    
    for (void* buffer : allocatedBuffers_) {
        if (buffer != nullptr) {
            free(buffer);
        }
    }
    allocatedBuffers_.clear();
}

sptr<MockSurfaceBuffer> SideOutputSurfaceProcessorUnitTest::CreateMockBufferWithMemory(uint32_t size)
{
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    if (size == 0 || size > MAX_BUFFER_SIZE) {
        EXPECT_CALL(*mockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*mockBuffer, GetSize()).WillRepeatedly(Return(0));
        return mockBuffer;
    }
    void* realBuffer = malloc(size);
    if (realBuffer != nullptr) {
        errno_t ret = memset_s(realBuffer, size, 0, size);
        if (ret != EOK) {
            free(realBuffer);
            EXPECT_CALL(*mockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
            EXPECT_CALL(*mockBuffer, GetSize()).WillRepeatedly(Return(0));
            return mockBuffer;
        }
        allocatedBuffers_.push_back(realBuffer);
        EXPECT_CALL(*mockBuffer, GetVirAddr()).WillRepeatedly(Return(realBuffer));
    } else {
        EXPECT_CALL(*mockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    }
    EXPECT_CALL(*mockBuffer, GetSize()).WillRepeatedly(Return(size));
    return mockBuffer;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Init_001
 * @tc.desc: test Init when already initialized
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Init_AlreadyInitialized, TestSize.Level1)
{
    EXPECT_CALL(*callback_, OnError(_)).Times(0);
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isInitialized_.load());

    ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Init_002
 * @tc.desc: test Init for first time initialization
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Init_FirstTime, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isInitialized_.load());
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isInitialized_.load());
    EXPECT_TRUE(processor_->isRunning_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Flush_001
 * @tc.desc: test Flush when producerSurface_ is not nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Flush_WithProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, CleanCache(false)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(true)).Times(AnyNumber());

    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Flush_002
 * @tc.desc: test Flush when producerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Flush_WithoutProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;
    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Flush_003
 * @tc.desc: test Flush when sideSurface_ is not nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Flush_WithSideSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sideSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, CleanCache(_)).Times(AnyNumber());

    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Flush_004
 * @tc.desc: test Flush when sideSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Flush_WithoutSideSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sideSurface_ = nullptr;
    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Stop_001
 * @tc.desc: test Stop normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Stop_Normal, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Stop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::STOPPING);
    EXPECT_TRUE(processor_->isPaused_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Start_001
 * @tc.desc: test Start when already in RUNNING state
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Start_AlreadyRunning, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::RUNNING);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_001
 * @tc.desc: test Release when processor is initialized
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_Initialized, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isInitialized_.load());

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(processor_->isInitialized_.load());
    EXPECT_FALSE(processor_->isRunning_.load());
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_002
 * @tc.desc: test Release when processor is in RUNNING state
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WhileRunning, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_003
 * @tc.desc: test Release when already released
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_AlreadyReleased, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isInitialized_.load());

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(processor_->isInitialized_.load());
    processor_ = nullptr;

    TestableSideOutputSurfaceProcessor tempProcessor;
    EXPECT_FALSE(tempProcessor.isInitialized_.load());
    ret = tempProcessor.Release();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_004
 * @tc.desc: test Release when worker thread is joinable
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WorkerJoinable, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->worker_.joinable());

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(processor_->worker_.joinable());
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_005
 * @tc.desc: test Release when sampleTask_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_NoSampleTask, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->sampleTask_, nullptr);

    processor_->sampleTask_ = nullptr;
    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_006
 * @tc.desc: test Release when consumerSurface_ is not nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, UnregisterConsumerListener()).Times(AnyNumber());

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->consumerSurface_, nullptr);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_007
 * @tc.desc: test Release when consumerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithoutConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = nullptr;

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_008
 * @tc.desc: test Release when producerSurface_ is not nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, Disconnect()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(true)).Times(AnyNumber());

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->producerSurface_, nullptr);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_009
 * @tc.desc: test Release when producerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithoutProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_001
 * @tc.desc: test SetOutputSurface with nullptr surface
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_NullSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->SetOutputSurface(nullptr);
    EXPECT_EQ(ret, Status::ERROR_NULL_SURFACE);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_002
 * @tc.desc: test SetOutputSurface with consumer surface
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_ConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurface> consumerMockSurface = new MockSurface();
    EXPECT_CALL(*consumerMockSurface, IsConsumer()).WillRepeatedly(Return(true));

    ret = processor_->SetOutputSurface(consumerMockSurface);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_003
 * @tc.desc: test SetOutputSurface with same surfaces
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_SameSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, GetUniqueId()).WillRepeatedly(Return(12345));
    EXPECT_CALL(*mockSurface_, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, Disconnect()).Times(AnyNumber());

    sptr<MockSurface> sameMockSurface = new MockSurface();
    EXPECT_CALL(*sameMockSurface, IsConsumer()).WillRepeatedly(Return(false));
    EXPECT_CALL(*sameMockSurface, GetUniqueId()).WillRepeatedly(Return(12345));
    EXPECT_CALL(*sameMockSurface, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*sameMockSurface, CleanCache(_)).Times(AnyNumber());

    ret = processor_->SetOutputSurface(sameMockSurface);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_005
 * @tc.desc: test SetOutputSurface when RegisterReleaseListener fails
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_RegisterFail, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;

    sptr<MockSurface> failMockSurface = new MockSurface();
    EXPECT_CALL(*failMockSurface, IsConsumer()).WillRepeatedly(Return(false));
    EXPECT_CALL(*failMockSurface, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*failMockSurface, RegisterReleaseListener(_)).WillRepeatedly(Return(GSERROR_NO_BUFFER));

    ret = processor_->SetOutputSurface(failMockSurface);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_001
 * @tc.desc: test OnConsumerBufferAvailable when isRunning_ is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    processor_->OnConsumerBufferAvailable();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_002
 * @tc.desc: test OnConsumerBufferAvailable when state is not RUNNING
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_NotRunningState, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isRunning_.load());
    EXPECT_NE(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::RUNNING);

    processor_->OnConsumerBufferAvailable();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_003
 * @tc.desc: test OnConsumerBufferAvailable when consumerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_NullConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::RUNNING);

    processor_->consumerSurface_ = nullptr;
    processor_->OnConsumerBufferAvailable();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_004
 * @tc.desc: test OnConsumerBufferAvailable when AcquireBuffer fails
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_AcquireFail, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, AcquireBuffer(_, _, _, _)).WillRepeatedly(Return(GSERROR_NO_BUFFER));
    EXPECT_CALL(*mockSurface_, UnregisterConsumerListener()).Times(AnyNumber());

    processor_->OnConsumerBufferAvailable();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_001
 * @tc.desc: test OnProducerBufferReleased when isRunning_ is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    GSError ret = processor_->OnProducerBufferReleased();
    EXPECT_EQ(ret, GSERROR_OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_002
 * @tc.desc: test OnProducerBufferReleased when state is STOPPING
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_Stopping, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->state_ = TestableSideOutputSurfaceProcessor::ProcessorState::STOPPING;

    GSError err = processor_->OnProducerBufferReleased();
    EXPECT_EQ(err, GSERROR_OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_003
 * @tc.desc: test OnProducerBufferReleased when isBufferQueueReady_ is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_QueueNotReady, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    processor_->isBufferQueueReady_ = false;

    GSError err = processor_->OnProducerBufferReleased();
    EXPECT_EQ(err, GSERROR_OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_004
 * @tc.desc: test OnProducerBufferReleased when producerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_NullProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    processor_->isBufferQueueReady_ = true;
    processor_->producerSurface_ = nullptr;

    GSError err = processor_->OnProducerBufferReleased();
    EXPECT_EQ(err, GSERROR_OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_005
 * @tc.desc: test OnProducerBufferReleased when RequestBuffer succeeds
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_RequestSuccess, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    processor_->isBufferQueueReady_ = true;
    processor_->producerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, Disconnect()).Times(AnyNumber());
    
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(mockBuffer), Return(GSERROR_OK)));

    size_t queueSizeBefore = processor_->producerBufferQueue_.size();
    GSError err = processor_->OnProducerBufferReleased();
    EXPECT_EQ(err, GSERROR_OK);
    EXPECT_EQ(processor_->producerBufferQueue_.size(), queueSizeBefore + 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_006
 * @tc.desc: test OnProducerBufferReleased when RequestBuffer fails
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_RequestFail, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    processor_->isBufferQueueReady_ = true;
    processor_->producerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, Disconnect()).Times(AnyNumber());
    
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(mockBuffer), Return(GSERROR_NO_BUFFER)));
    EXPECT_CALL(*mockSurface_, CancelBuffer(_)).Times(AnyNumber());

    size_t queueSizeBefore = processor_->producerBufferQueue_.size();
    GSError err = processor_->OnProducerBufferReleased();
    EXPECT_EQ(err, GSERROR_NO_BUFFER);
    EXPECT_EQ(processor_->producerBufferQueue_.size(), queueSizeBefore);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffer_001
 * @tc.desc: test ProcessBuffer with valid memory addresses (memcpy success path)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffer_Success, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = CreateMockBufferWithMemory(1024);
    sptr<MockSurfaceBuffer> dstMockBuffer = CreateMockBufferWithMemory(2048);

    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*dstMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*dstMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*dstMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    srcBufferInfo.timestamp = 1000;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    dstBufferInfo.buffer = dstMockBuffer;
    dstBufferInfo.timestamp = 0;

    processor_->consumerSurface_ = nullptr;
    
    processor_->callback_ = nullptr;
    bool result = processor_->ProcessBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffer_002
 * @tc.desc: test ProcessBuffer with consumerSurface
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffer_WithConsumerSurface, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = CreateMockBufferWithMemory(1024);
    sptr<MockSurfaceBuffer> dstMockBuffer = CreateMockBufferWithMemory(2048);

    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*dstMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*dstMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*dstMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    srcBufferInfo.timestamp = 1000;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    dstBufferInfo.buffer = dstMockBuffer;
    dstBufferInfo.timestamp = 0;

    processor_->consumerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, ReleaseBuffer(_, -1)).WillRepeatedly(Return(GSERROR_OK));
    EXPECT_CALL(*mockSurface_, UnregisterConsumerListener()).Times(AnyNumber());

    processor_->callback_ = nullptr;
    bool result = processor_->ProcessBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffer_003
 * @tc.desc: test ProcessBuffer with null memory addresses (null pointer path)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffer_NullMemory, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = new MockSurfaceBuffer();
    sptr<MockSurfaceBuffer> dstMockBuffer = new MockSurfaceBuffer();

    EXPECT_CALL(*srcMockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*dstMockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*srcMockBuffer, GetSize()).WillRepeatedly(Return(100));
    EXPECT_CALL(*dstMockBuffer, GetSize()).WillRepeatedly(Return(200));
    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*dstMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*dstMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*dstMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    srcBufferInfo.timestamp = 1000;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    dstBufferInfo.buffer = dstMockBuffer;
    dstBufferInfo.timestamp = 0;

    processor_->consumerSurface_ = nullptr;
    
    processor_->callback_ = nullptr;
    bool result = processor_->ProcessBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OutputBuffer_001
 * @tc.desc: test OutputBuffer with callback set
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OutputBuffer_WithCallback, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->SetCallback(callback_);
    EXPECT_NE(processor_->callback_, nullptr);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    EXPECT_CALL(*callback_, OnOutputBufferAvailable(1, _, false)).Times(AnyNumber());

    processor_->OutputBuffer(bufferInfo);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OutputBuffer_002
 * @tc.desc: test OutputBuffer without callback set
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OutputBuffer_WithoutCallback, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->callback_ = nullptr;

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    EXPECT_CALL(*callback_, OnOutputBufferAvailable(_, _, _)).Times(0);

    processor_->OutputBuffer(bufferInfo);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OutputBuffer_003
 * @tc.desc: test OutputBuffer when renderBufferQueue overflow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OutputBuffer_Overflow, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    for (size_t i = 0; i < 6; i++) {
        sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
        EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(i));
        
        TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = mockBuffer;
        bufferInfo.timestamp = i;
        processor_->renderBufferQueue_[i] = bufferInfo;
    }

    EXPECT_GE(processor_->renderBufferQueue_.size(), static_cast<size_t>(6));

    sptr<MockSurfaceBuffer> newMockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*newMockBuffer, GetSeqNum()).WillRepeatedly(Return(100));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo newBufferInfo;
    newBufferInfo.buffer = newMockBuffer;
    newBufferInfo.timestamp = 100;

    processor_->producerSurface_ = nullptr;
    processor_->OutputBuffer(newBufferInfo);
    EXPECT_LE(processor_->renderBufferQueue_.size(), static_cast<size_t>(6));
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RenderOutputBufferInner_001
 * @tc.desc: test RenderOutputBufferInner when isRunning_ is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferInner_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    Status ret = processor_->RenderOutputBufferInner(0, -1, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RenderOutputBufferInner_002
 * @tc.desc: test RenderOutputBufferInner with invalid index
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferInner_InvalidIndex, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isRunning_.load());

    processor_->renderBufferQueue_.clear();

    ret = processor_->RenderOutputBufferInner(999, -1, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RenderOutputBufferInner_003
 * @tc.desc: test RenderOutputBufferInner when render is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferInner_NotRender, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;
    processor_->renderBufferQueue_[1] = bufferInfo;

    ret = processor_->RenderOutputBufferInner(1, -1, false);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->producerBufferQueue_.size(), static_cast<size_t>(1));
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FindBufferFromRenderQueue_001
 * @tc.desc: test FindBufferFromRenderQueue when buffer not found
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FindBufferFromRenderQueue_NotFound, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->renderBufferQueue_.clear();

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bool result = processor_->FindBufferFromRenderQueue(999, bufferInfo);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FindBufferFromRenderQueue_002
 * @tc.desc: test FindBufferFromRenderQueue when buffer found by index
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FindBufferFromRenderQueue_FoundByIndex, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    uint32_t seqNum = 1;
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(seqNum));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;
    processor_->renderBufferQueue_[seqNum] = bufferInfo;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo foundBufferInfo;
    bool result = processor_->FindBufferFromRenderQueue(seqNum, foundBufferInfo);
    EXPECT_TRUE(result);
    EXPECT_TRUE(processor_->renderBufferQueue_.empty());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FindBufferFromRenderQueue_003
 * @tc.desc: test FindBufferFromRenderQueue when buffer found by SeqNum in loop
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FindBufferFromRenderQueue_FoundBySeqNum, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    uint32_t realSeqNum = 10;
    uint32_t fakeKey = 9999;
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(realSeqNum));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;
    processor_->renderBufferQueue_[fakeKey] = bufferInfo;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo foundBufferInfo;
    bool result = processor_->FindBufferFromRenderQueue(realSeqNum, foundBufferInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FlushBufferToProducer_001
 * @tc.desc: test FlushBufferToProducer when producerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FlushBufferToProducer_NullProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    processor_->producerSurface_ = nullptr;
    Status flushRet = processor_->FlushBufferToProducer(bufferInfo, -1);
    EXPECT_EQ(flushRet, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FlushBufferToProducer_002
 * @tc.desc: test FlushBufferToProducer when FlushBuffer succeeds
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FlushBufferToProducer_Success, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 5000;

    processor_->producerSurface_ = mockSurface_;
    processor_->isBufferQueueReady_ = true;
    EXPECT_CALL(*mockSurface_, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, Disconnect()).Times(AnyNumber());
    
    EXPECT_CALL(*mockSurface_, FlushBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));

    Status flushRet = processor_->FlushBufferToProducer(bufferInfo, -1);
    EXPECT_EQ(flushRet, Status::OK);
    EXPECT_EQ(processor_->pendingSideTimestamp_, 5000);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FlushBufferToProducer_003
 * @tc.desc: test FlushBufferToProducer when FlushBuffer fails
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FlushBufferToProducer_FlushFail, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetSeqNum()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    processor_->producerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, UnRegisterReleaseListener()).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CleanCache(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, Disconnect()).Times(AnyNumber());
    
    EXPECT_CALL(*mockSurface_, FlushBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_INVALID_ARGUMENTS));

    Status flushRet = processor_->FlushBufferToProducer(bufferInfo, -1);
    EXPECT_EQ(flushRet, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_CopyBufferToSideSurface_001
 * @tc.desc: test CopyBufferToSideSurface when srcBuffer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, CopyBufferToSideSurface_NullSrcBuffer, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    bool result = processor_->CopyBufferToSideSurface(nullptr, 1000);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_CopyBufferToSideSurface_002
 * @tc.desc: test CopyBufferToSideSurface when sideSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, CopyBufferToSideSurface_NullSideSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(100));
    EXPECT_CALL(*mockBuffer, GetFormat()).WillRepeatedly(Return(1));

    processor_->sideSurface_ = nullptr;
    bool result = processor_->CopyBufferToSideSurface(mockBuffer, 1000);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_CopyBufferToSideSurface_003
 * @tc.desc: test CopyBufferToSideSurface success path with valid memory
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, CopyBufferToSideSurface_RequestSuccess, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> srcMockBuffer = CreateMockBufferWithMemory(100);
    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*srcMockBuffer, GetUsage()).WillRepeatedly(Return(BUFFER_USAGE_CPU_READ));

    sptr<MockSurfaceBuffer> sideMockBuffer = CreateMockBufferWithMemory(200);
    EXPECT_CALL(*sideMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*sideMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*sideMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    processor_->sideSurface_ = mockSurface_;
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(sideMockBuffer), Return(GSERROR_OK)));
    EXPECT_CALL(*mockSurface_, FlushBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));

    bool result = processor_->CopyBufferToSideSurface(srcMockBuffer, 1000);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_CopyBufferToSideSurface_004
 * @tc.desc: test CopyBufferToSideSurface with null virAddr (null pointer path)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, CopyBufferToSideSurface_NullVirAddr, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> srcMockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*srcMockBuffer, GetUsage()).WillRepeatedly(Return(BUFFER_USAGE_CPU_READ));
    EXPECT_CALL(*srcMockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*srcMockBuffer, GetSize()).WillRepeatedly(Return(100));

    sptr<MockSurfaceBuffer> sideMockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*sideMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*sideMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*sideMockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*sideMockBuffer, GetSize()).WillRepeatedly(Return(200));
    EXPECT_CALL(*sideMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    processor_->sideSurface_ = mockSurface_;
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(sideMockBuffer), Return(GSERROR_OK)));
    EXPECT_CALL(*mockSurface_, CancelBuffer(_)).Times(AnyNumber());

    bool result = processor_->CopyBufferToSideSurface(srcMockBuffer, 1000);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RequestSideBuffer_001
 * @tc.desc: test RequestSideBuffer when RequestBuffer succeeds
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RequestSideBuffer_Success, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(mockBuffer), Return(GSERROR_OK)));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo sideBufferInfo;
    bool result = processor_->RequestSideBuffer(sideBufferInfo, mockSurface_);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RequestSideBuffer_002
 * @tc.desc: test RequestSideBuffer when RequestBuffer fails
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RequestSideBuffer_RequestFail, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(mockBuffer), Return(GSERROR_NO_BUFFER)));
    EXPECT_CALL(*mockSurface_, CancelBuffer(_)).Times(AnyNumber());

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo sideBufferInfo;
    bool result = processor_->RequestSideBuffer(sideBufferInfo, mockSurface_);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RequestSideBuffer_003
 * @tc.desc: test RequestSideBuffer when cfg.width and cfg.height are 0
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RequestSideBuffer_DefaultCfg, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    
    processor_->requestCfg_.width = 0;
    processor_->requestCfg_.height = 0;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(mockBuffer), Return(GSERROR_OK)));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo sideBufferInfo;
    bool result = processor_->RequestSideBuffer(sideBufferInfo, mockSurface_);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FlushSideBuffer_001
 * @tc.desc: test FlushSideBuffer when FlushBuffer succeeds
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FlushSideBuffer_Success, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(1080));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    int64_t timestamp = 1000;

    EXPECT_CALL(*mockSurface_, FlushBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));

    bool result = processor_->FlushSideBuffer(bufferInfo, timestamp, mockSurface_);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FlushSideBuffer_002
 * @tc.desc: test FlushSideBuffer when FlushBuffer fails
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FlushSideBuffer_FlushFail, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(1080));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    int64_t timestamp = 1000;

    EXPECT_CALL(*mockSurface_, FlushBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_INVALID_ARGUMENTS));

    bool result = processor_->FlushSideBuffer(bufferInfo, timestamp, mockSurface_);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_001
 * @tc.desc: test HandleSampleTask when pendingSideSrcBuffer_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_NullPendingBuffer, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->pendingSideSrcBuffer_ = nullptr;

    processor_->HandleSampleTask();
    EXPECT_TRUE(processor_->sampleDone_.load());
    EXPECT_EQ(processor_->sampleResult_, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_002
 * @tc.desc: test HandleSampleTask when sideSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_NullSideSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*mockBuffer, GetFormat()).WillRepeatedly(Return(1));

    processor_->pendingSideSrcBuffer_ = mockBuffer;
    processor_->pendingSideTimestamp_ = 1000;
    processor_->sideSurface_ = nullptr;

    processor_->HandleSampleTask();
    EXPECT_TRUE(processor_->sampleDone_.load());
    EXPECT_EQ(processor_->sampleResult_, 0);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_003
 * @tc.desc: test HandleSampleTask when copy succeeds with valid memory
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_CopySuccess, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> srcMockBuffer = CreateMockBufferWithMemory(100);
    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*srcMockBuffer, GetUsage()).WillRepeatedly(Return(BUFFER_USAGE_CPU_READ));

    sptr<MockSurfaceBuffer> sideMockBuffer = CreateMockBufferWithMemory(200);
    EXPECT_CALL(*sideMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*sideMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*sideMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    processor_->pendingSideSrcBuffer_ = srcMockBuffer;
    processor_->pendingSideTimestamp_ = 1000;
    processor_->sideSurface_ = mockSurface_;
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(sideMockBuffer), Return(GSERROR_OK)));
    EXPECT_CALL(*mockSurface_, FlushBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));

    processor_->HandleSampleTask();
    EXPECT_TRUE(processor_->sampleDone_.load());
    EXPECT_EQ(processor_->sampleResult_, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_004
 * @tc.desc: test HandleSampleTask with null virAddr (null pointer path)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_NullVirAddr, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> srcMockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*srcMockBuffer, GetUsage()).WillRepeatedly(Return(BUFFER_USAGE_CPU_READ));
    EXPECT_CALL(*srcMockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*srcMockBuffer, GetSize()).WillRepeatedly(Return(100));

    sptr<MockSurfaceBuffer> sideMockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*sideMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*sideMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*sideMockBuffer, GetVirAddr()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(*sideMockBuffer, GetSize()).WillRepeatedly(Return(200));
    EXPECT_CALL(*sideMockBuffer, GetFormat()).WillRepeatedly(Return(1));

    processor_->pendingSideSrcBuffer_ = srcMockBuffer;
    processor_->pendingSideTimestamp_ = 1000;
    processor_->sideSurface_ = mockSurface_;
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;
    
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<0>(sideMockBuffer), Return(GSERROR_OK)));
    EXPECT_CALL(*mockSurface_, CancelBuffer(_)).Times(AnyNumber());

    processor_->HandleSampleTask();
    EXPECT_TRUE(processor_->sampleDone_.load());
    EXPECT_EQ(processor_->sampleResult_, 0);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_DealNoImage_001
 * @tc.desc: test DealNoImage normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNoImage_Normal, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->DealNoImage();
    EXPECT_TRUE(processor_->sampleDone_.load());
    EXPECT_EQ(processor_->sampleResult_, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetVideoSample_001
 * @tc.desc: test GetVideoSample when sampleProcessing_ is true
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_AlreadyProcessing, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sampleProcessing_ = true;

    int32_t result = -1;
    processor_->GetVideoSample(result);
    EXPECT_EQ(result, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetVideoSample_002
 * @tc.desc: test GetVideoSample with sampleTask
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_WithTask, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->sampleTask_, nullptr);

    processor_->pendingSideSrcBuffer_ = nullptr;

    int32_t result = -1;
    processor_->GetVideoSample(result);
    EXPECT_EQ(result, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetVideoSample_003
 * @tc.desc: test GetVideoSample without sampleTask
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_WithoutTask, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sampleTask_ = nullptr;
    processor_->pendingSideSrcBuffer_ = nullptr;

    int32_t result = -1;
    processor_->GetVideoSample(result);
    EXPECT_EQ(result, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ClearBufferQueues_001
 * @tc.desc: test ClearBufferQueues with consumerSurface
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ClearBufferQueues_WithConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer1 = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer1, GetSeqNum()).WillRepeatedly(Return(1));
    
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer1;
    processor_->consumerBufferQueue_.push(bufferInfo);

    processor_->consumerSurface_ = mockSurface_;
    EXPECT_CALL(*mockSurface_, ReleaseBuffer(_, -1)).WillRepeatedly(Return(GSERROR_OK));

    processor_->ClearBufferQueues();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
    EXPECT_FALSE(processor_->isBufferQueueReady_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ClearBufferQueues_002
 * @tc.desc: test ClearBufferQueues without consumerSurface
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ClearBufferQueues_WithoutConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer1 = new MockSurfaceBuffer();
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer1;
    processor_->consumerBufferQueue_.push(bufferInfo);

    processor_->consumerSurface_ = nullptr;

    processor_->ClearBufferQueues();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
    EXPECT_FALSE(processor_->isBufferQueueReady_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ClearBufferQueues_003
 * @tc.desc: test ClearBufferQueues with pendingBuffer
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ClearBufferQueues_WithPendingBuffer, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    processor_->pendingSideSrcBuffer_ = mockBuffer;
    processor_->pendingSideTimestamp_ = 1000;

    processor_->ClearBufferQueues();
    EXPECT_FALSE(processor_->isBufferQueueReady_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_001
 * @tc.desc: test GetConsumerAndProducerBuffer when queues are empty
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_EmptyQueue, TestSize.Level1)
{
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
    EXPECT_TRUE(processor_->producerBufferQueue_.empty());

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;

    bool ret = processor_->GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_002
 * @tc.desc: test GetConsumerAndProducerBuffer when srcBuffer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_NullSrcBuffer, TestSize.Level1)
{
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo nullBuffer;
    nullBuffer.buffer = nullptr;
    processor_->consumerBufferQueue_.push(nullBuffer);
    processor_->producerBufferQueue_.push(nullBuffer);

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    bool ret = processor_->GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_003
 * @tc.desc: test GetConsumerAndProducerBuffer when dstBuffer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_NullDstBuffer, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = new MockSurfaceBuffer();
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    processor_->consumerBufferQueue_.push(srcBufferInfo);

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo nullBuffer;
    nullBuffer.buffer = nullptr;
    processor_->producerBufferQueue_.push(nullBuffer);

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    bool ret = processor_->GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_004
 * @tc.desc: test GetConsumerAndProducerBuffer normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_Success, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = new MockSurfaceBuffer();
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    srcBufferInfo.timestamp = 1000;
    processor_->consumerBufferQueue_.push(srcBufferInfo);

    sptr<MockSurfaceBuffer> dstMockBuffer = new MockSurfaceBuffer();
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    dstBufferInfo.buffer = dstMockBuffer;
    dstBufferInfo.timestamp = 2000;
    processor_->producerBufferQueue_.push(dstBufferInfo);

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo resultSrcBufferInfo;
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo resultDstBufferInfo;
    bool ret = processor_->GetConsumerAndProducerBuffer(resultSrcBufferInfo, resultDstBufferInfo);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_PushBufferToConsumer_001
 * @tc.desc: test PushBufferToConsumer when buffer queue not ready
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, PushBufferToConsumer_QueueNotReady, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*mockBuffer, GetFormat()).WillRepeatedly(Return(1));
    EXPECT_CALL(*mockBuffer, GetUsage()).WillRepeatedly(Return(BUFFER_USAGE_CPU_READ));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    processor_->isBufferQueueReady_ = false;
    processor_->isRunning_ = true;

    processor_->PushBufferToConsumer(bufferInfo);
    EXPECT_TRUE(processor_->isBufferQueueReady_.load());
    EXPECT_EQ(processor_->requestCfg_.width, 1920);
    EXPECT_EQ(processor_->requestCfg_.height, 1080);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_PushBufferToConsumer_002
 * @tc.desc: test PushBufferToConsumer when buffer queue already ready
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, PushBufferToConsumer_QueueAlreadyReady, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    processor_->isBufferQueueReady_ = true;
    processor_->requestCfg_.width = 1920;
    processor_->requestCfg_.height = 1080;

    size_t queueSizeBefore = processor_->consumerBufferQueue_.size();
    processor_->PushBufferToConsumer(bufferInfo);
    EXPECT_EQ(processor_->consumerBufferQueue_.size(), queueSizeBefore + 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_DealNotRenderBuffer_001
 * @tc.desc: test DealNotRenderBuffer when state is not IDLE
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNotRenderBuffer_NotIdle, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::IDLE);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    size_t queueSizeBefore = processor_->producerBufferQueue_.size();
    processor_->DealNotRenderBuffer(bufferInfo);
    EXPECT_EQ(processor_->producerBufferQueue_.size(), queueSizeBefore + 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_DealNotRenderBuffer_002
 * @tc.desc: test DealNotRenderBuffer when state is IDLE
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNotRenderBuffer_Idle, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    processor_->state_ = TestableSideOutputSurfaceProcessor::ProcessorState::IDLE;
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::IDLE);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    size_t queueSizeBefore = processor_->producerBufferQueue_.size();
    processor_->DealNotRenderBuffer(bufferInfo);
    EXPECT_EQ(processor_->producerBufferQueue_.size(), queueSizeBefore + 1);
    EXPECT_EQ(processor_->state_.load(), TestableSideOutputSurfaceProcessor::ProcessorState::IDLE);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_WaitTrigger_001
 * @tc.desc: test WaitTrigger when isRunning_ is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, WaitTrigger_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    bool ret = processor_->WaitTrigger();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffers_001
 * @tc.desc: test ProcessBuffers when isRunning_ is false
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffers_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    processor_->ProcessBuffers();
    EXPECT_FALSE(processor_->isProcessing_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffers_002
 * @tc.desc: test ProcessBuffers when consumerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffers_NullConsumerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = nullptr;

    processor_->ProcessBuffers();
    EXPECT_FALSE(processor_->isProcessing_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffers_003
 * @tc.desc: test ProcessBuffers when producerSurface_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffers_NullProducerSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;

    processor_->ProcessBuffers();
    EXPECT_FALSE(processor_->isProcessing_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetVideoWindowSize_001
 * @tc.desc: test SetVideoWindowSize normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetVideoWindowSize_Normal, TestSize.Level1)
{
    Status ret = processor_->SetVideoWindowSize(1920, 1080);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetParameter_001
 * @tc.desc: test SetParameter normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetParameter_Normal, TestSize.Level1)
{
    Format format;
    Status ret = processor_->SetParameter(format);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetPostProcessorOn_001
 * @tc.desc: test SetPostProcessorOn normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetPostProcessorOn_Normal, TestSize.Level1)
{
    Status ret = processor_->SetPostProcessorOn(true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetCallback_001
 * @tc.desc: test SetCallback normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetCallback_Normal, TestSize.Level1)
{
    Status ret = processor_->SetCallback(callback_);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->callback_, callback_);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetEventReceiver_001
 * @tc.desc: test SetEventReceiver normal flow
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetEventReceiver_Normal, TestSize.Level1)
{
    Status ret = processor_->SetEventReceiver(receiver_);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->eventReceiver_, receiver_);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_PushBufferToConsumer_003
 * @tc.desc: test PushBufferToConsumer when resolution changed
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, PushBufferToConsumer_ResolutionChanged, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*mockBuffer, GetFormat()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    processor_->isBufferQueueReady_ = true;
    processor_->requestCfg_.width = 320;
    processor_->requestCfg_.height = 180;
    processor_->producerSurface_ = mockSurface_;

    sptr<MockSurfaceBuffer> oldBuffer = new MockSurfaceBuffer();
    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo oldBufferInfo;
    oldBufferInfo.buffer = oldBuffer;
    processor_->producerBufferQueue_.push(oldBufferInfo);

    EXPECT_CALL(*mockSurface_, SetDefaultWidthAndHeight(1920, 1080)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, CancelBuffer(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));

    processor_->PushBufferToConsumer(bufferInfo);
    EXPECT_EQ(processor_->requestCfg_.width, 1920);
    EXPECT_EQ(processor_->requestCfg_.height, 1080);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_PushBufferToConsumer_004
 * @tc.desc: test PushBufferToConsumer when resolution changed but producerSurface is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, PushBufferToConsumer_ResolutionChangedNullSurface, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<MockSurfaceBuffer> mockBuffer = new MockSurfaceBuffer();
    EXPECT_CALL(*mockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*mockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*mockBuffer, GetFormat()).WillRepeatedly(Return(1));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = mockBuffer;
    bufferInfo.timestamp = 1000;

    processor_->isBufferQueueReady_ = true;
    processor_->requestCfg_.width = 320;
    processor_->requestCfg_.height = 180;
    processor_->producerSurface_ = nullptr;

    processor_->PushBufferToConsumer(bufferInfo);
    EXPECT_EQ(processor_->requestCfg_.width, 1920);
    EXPECT_EQ(processor_->requestCfg_.height, 1080);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffer_004
 * @tc.desc: test ProcessBuffer when buffer size mismatch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffer_SizeMismatch, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = CreateMockBufferWithMemory(3137536);
    sptr<MockSurfaceBuffer> dstMockBuffer = CreateMockBufferWithMemory(94208);

    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(24));
    EXPECT_CALL(*dstMockBuffer, GetWidth()).WillRepeatedly(Return(320));
    EXPECT_CALL(*dstMockBuffer, GetHeight()).WillRepeatedly(Return(180));
    EXPECT_CALL(*dstMockBuffer, GetFormat()).WillRepeatedly(Return(24));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    srcBufferInfo.timestamp = 1000;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    dstBufferInfo.buffer = dstMockBuffer;
    dstBufferInfo.timestamp = 0;

    processor_->consumerSurface_ = mockSurface_;
    processor_->producerSurface_ = mockSurface_;
    processor_->isBufferQueueReady_ = true;

    EXPECT_CALL(*mockSurface_, ReleaseBuffer(_, -1)).WillRepeatedly(Return(GSERROR_OK));
    EXPECT_CALL(*mockSurface_, CancelBuffer(_)).Times(AnyNumber());
    EXPECT_CALL(*mockSurface_, RequestBuffer(_, _, _)).WillRepeatedly(Return(GSERROR_OK));

    bool result = processor_->ProcessBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffer_005
 * @tc.desc: test ProcessBuffer when buffer size mismatch with null surfaces
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffer_SizeMismatchNullSurfaces, TestSize.Level1)
{
    sptr<MockSurfaceBuffer> srcMockBuffer = CreateMockBufferWithMemory(3137536);
    sptr<MockSurfaceBuffer> dstMockBuffer = CreateMockBufferWithMemory(94208);

    EXPECT_CALL(*srcMockBuffer, GetWidth()).WillRepeatedly(Return(1920));
    EXPECT_CALL(*srcMockBuffer, GetHeight()).WillRepeatedly(Return(1080));
    EXPECT_CALL(*srcMockBuffer, GetFormat()).WillRepeatedly(Return(24));
    EXPECT_CALL(*dstMockBuffer, GetWidth()).WillRepeatedly(Return(320));
    EXPECT_CALL(*dstMockBuffer, GetHeight()).WillRepeatedly(Return(180));
    EXPECT_CALL(*dstMockBuffer, GetFormat()).WillRepeatedly(Return(24));

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    srcBufferInfo.buffer = srcMockBuffer;
    srcBufferInfo.timestamp = 1000;

    TestableSideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    dstBufferInfo.buffer = dstMockBuffer;
    dstBufferInfo.timestamp = 0;

    processor_->consumerSurface_ = nullptr;
    processor_->producerSurface_ = nullptr;
    processor_->isBufferQueueReady_ = true;

    bool result = processor_->ProcessBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(result);
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS