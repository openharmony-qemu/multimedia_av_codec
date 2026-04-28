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

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {

void SideOutputSurfaceProcessorUnitTest::SetUpTestCase(void) {}

void SideOutputSurfaceProcessorUnitTest::TearDownTestCase(void) {}

void SideOutputSurfaceProcessorUnitTest::SetUp(void)
{
    processor_ = std::make_shared<SideOutputSurfaceProcessor>();
    EXPECT_NE(processor_, nullptr);
    callback_ = std::make_shared<MockPostProcessorCallback>();
    receiver_ = std::make_shared<MockEventReceiver>();
}

void SideOutputSurfaceProcessorUnitTest::TearDown(void)
{
    if (processor_ != nullptr) {
        processor_->Release();
        processor_ = nullptr;
    }
    callback_ = nullptr;
    receiver_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Init_001
 * @tc.desc: test Init when already initialized (branch: isInitialized_.load() == true)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Init_AlreadyInitialized_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isInitialized_.load());

    ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Init_002
 * @tc.desc: test Init for first time (branch: isInitialized_.load() == false)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Init_FirstTime_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isInitialized_.load());

    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isInitialized_.load());
    EXPECT_TRUE(processor_->isRunning_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Flush_001
 * @tc.desc: test Flush when producerSurface_ != nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Flush_WithProducerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> inputSurface = processor_->GetInputSurface();
    EXPECT_NE(inputSurface, nullptr);
    EXPECT_NE(processor_->producerSurface_, nullptr);

    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Flush_002
 * @tc.desc: test Flush when producerSurface_ == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Flush_WithoutProducerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;
    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Stop_001
 * @tc.desc: test Stop normal branch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Stop_Normal_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->Stop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::STOPPING);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Start_001
 * @tc.desc: test Start when state == RUNNING (branch: already running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Start_AlreadyRunning_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Start_002
 * @tc.desc: test Start when state != RUNNING (branch: from idle)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Start_FromIdle_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_001
 * @tc.desc: test Release when not initialized (branch: !isInitialized_.load() == true)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_NotInitialized_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isInitialized_.load());

    Status ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_002
 * @tc.desc: test Release when initialized (branch: !isInitialized_.load() == false)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_Initialized_Branch, TestSize.Level1)
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
 * @tc.name: SideOutputSurfaceProcessor_Release_003
 * @tc.desc: test Release when state == RUNNING (branch: state == RUNNING)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WhileRunning_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_004
 * @tc.desc: test Release when state != RUNNING (branch: state != RUNNING)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_NotRunningState_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_005
 * @tc.desc: test Release when consumerSurface_ != nullptr (branch: consumerSurface_ != nullptr)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> inputSurface = processor_->GetInputSurface();
    EXPECT_NE(inputSurface, nullptr);
    EXPECT_NE(processor_->consumerSurface_, nullptr);

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->consumerSurface_, nullptr);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_006
 * @tc.desc: test Release when consumerSurface_ == nullptr (branch: consumerSurface_ == nullptr)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithoutConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = nullptr;

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_007
 * @tc.desc: test Release when producerSurface_ != nullptr (branch: producerSurface_ != nullptr)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithProducerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> inputSurface = processor_->GetInputSurface();
    EXPECT_NE(inputSurface, nullptr);
    EXPECT_NE(processor_->producerSurface_, nullptr);

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->producerSurface_, nullptr);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_Release_008
 * @tc.desc: test Release when producerSurface_ == nullptr (branch: producerSurface_ == nullptr)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, Release_WithoutProducerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetInputSurface_001
 * @tc.desc: test GetInputSurface when consumerSurface_ != nullptr (branch: already exists)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetInputSurface_AlreadyExist_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> surface1 = processor_->GetInputSurface();
    EXPECT_NE(surface1, nullptr);
    EXPECT_NE(processor_->consumerSurface_, nullptr);

    sptr<Surface> surface2 = processor_->GetInputSurface();
    EXPECT_EQ(surface2, surface1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetInputSurface_002
 * @tc.desc: test GetInputSurface when consumerSurface_ == nullptr (branch: create new)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetInputSurface_CreateNew_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->consumerSurface_, nullptr);

    sptr<Surface> surface = processor_->GetInputSurface();
    EXPECT_NE(surface, nullptr);
    EXPECT_NE(processor_->consumerSurface_, nullptr);
    EXPECT_NE(processor_->producerSurface_, nullptr);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_001
 * @tc.desc: test SetOutputSurface when surface == nullptr (branch: null surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_NullSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->SetOutputSurface(nullptr);
    EXPECT_EQ(ret, Status::ERROR_NULL_SURFACE);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_002
 * @tc.desc: test SetOutputSurface when producerSurface_ == nullptr (branch: no existing surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_NoExistingSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestConsumer");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> outputSurface = Surface::CreateSurfaceAsProducer(producer);

    ret = processor_->SetOutputSurface(outputSurface);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_003
 * @tc.desc: test SetOutputSurface when same surface (branch: GetUniqueId equal)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_SameSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestSame");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> outputSurface = Surface::CreateSurfaceAsProducer(producer);

    ret = processor_->SetOutputSurface(outputSurface);
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->SetOutputSurface(outputSurface);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetOutputSurface_004
 * @tc.desc: test SetOutputSurface when different surface (branch: GetUniqueId not equal)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetOutputSurface_DifferentSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> consumerSurface1 = Surface::CreateSurfaceAsConsumer("TestDiff1");
    sptr<IBufferProducer> producer1 = consumerSurface1->GetProducer();
    sptr<Surface> outputSurface1 = Surface::CreateSurfaceAsProducer(producer1);
    ret = processor_->SetOutputSurface(outputSurface1);
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> consumerSurface2 = Surface::CreateSurfaceAsConsumer("TestDiff2");
    sptr<IBufferProducer> producer2 = consumerSurface2->GetProducer();
    sptr<Surface> outputSurface2 = Surface::CreateSurfaceAsProducer(producer2);
    ret = processor_->SetOutputSurface(outputSurface2);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_001
 * @tc.desc: test OnConsumerBufferAvailable when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_NotRunning_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    processor_->OnConsumerBufferAvailable();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_002
 * @tc.desc: test OnConsumerBufferAvailable when state != RUNNING (branch: not running state)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_NotRunningState_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(processor_->isRunning_.load());
    EXPECT_NE(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);

    processor_->OnConsumerBufferAvailable();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_003
 * @tc.desc: test OnConsumerBufferAvailable when consumerSurface == nullptr (branch: null surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_NullConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::RUNNING);

    processor_->consumerSurface_ = nullptr;
    processor_->OnConsumerBufferAvailable();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnConsumerBufferAvailable_004
 * @tc.desc: test OnConsumerBufferAvailable when consumerSurface != nullptr (branch: valid surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnConsumerBufferAvailable_ValidConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> inputSurface = processor_->GetInputSurface();
    EXPECT_NE(inputSurface, nullptr);
    EXPECT_NE(processor_->consumerSurface_, nullptr);

    processor_->OnConsumerBufferAvailable();
}

HWTEST_F(SideOutputSurfaceProcessorUnitTest, PushBufferToConsumer_AlreadyReady_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->isBufferQueueReady_ = true;

    SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.timestamp = 1000;
    processor_->PushBufferToConsumer(bufferInfo);
}

HWTEST_F(SideOutputSurfaceProcessorUnitTest, PushBufferToConsumer_NeedPrepare_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->isBufferQueueReady_ = true;
    processor_->needPrepareBuffers_ = true;

    SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.timestamp = 1000;
    processor_->PushBufferToConsumer(bufferInfo);

    EXPECT_FALSE(processor_->needPrepareBuffers_.load());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_001
 * @tc.desc: test OnProducerBufferReleased when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_NotRunning_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    GSError ret = processor_->OnProducerBufferReleased();
    EXPECT_EQ(ret, GSERROR_OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_002
 * @tc.desc: test OnProducerBufferReleased when state == STOPPING (branch: stopping)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_Stopping_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->state_ = SideOutputSurfaceProcessor::ProcessorState::STOPPING;

    GSError err = processor_->OnProducerBufferReleased();
    EXPECT_EQ(err, GSERROR_OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OnProducerBufferReleased_003
 * @tc.desc: test OnProducerBufferReleased when !isBufferQueueReady_ (branch: queue not ready)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_QueueNotReady_Branch, TestSize.Level1)
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
 * @tc.desc: test OnProducerBufferReleased when producerSurface == nullptr (branch: null surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OnProducerBufferReleased_NullProducerSurface_Branch, TestSize.Level1)
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
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_001
 * @tc.desc: test GetConsumerAndProducerBuffer when queues empty (branch: empty queue)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_EmptyQueue_Branch, TestSize.Level1)
{
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
    EXPECT_TRUE(processor_->producerBufferQueue_.empty());

    SideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    SideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;

    bool ret = processor_->GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_002
 * @tc.desc: test GetConsumerAndProducerBuffer when srcBufferInfo.buffer == nullptr (branch: null src buffer)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_NullSrcBuffer_Branch, TestSize.Level1)
{
    SideOutputSurfaceProcessor::SurfaceBufferInfo nullBuffer;
    nullBuffer.buffer = nullptr;
    processor_->consumerBufferQueue_.push(nullBuffer);
    processor_->producerBufferQueue_.push(nullBuffer);

    SideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    SideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    bool ret = processor_->GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetConsumerAndProducerBuffer_003
 * @tc.desc: test GetConsumerAndProducerBuffer when dstBufferInfo.buffer == nullptr (branch: null dst buffer)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetConsumerAndProducerBuffer_NullDstBuffer_Branch, TestSize.Level1)
{
    SideOutputSurfaceProcessor::SurfaceBufferInfo nullBuffer;
    nullBuffer.buffer = nullptr;
    processor_->consumerBufferQueue_.push(nullBuffer);
    processor_->producerBufferQueue_.push(nullBuffer);

    SideOutputSurfaceProcessor::SurfaceBufferInfo srcBufferInfo;
    SideOutputSurfaceProcessor::SurfaceBufferInfo dstBufferInfo;
    bool ret = processor_->GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_CopyToSideSurface_001
 * @tc.desc: test CopyToSideSurface when sideSurface_ == nullptr (branch: null side surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, CopyToSideSurface_NullSideSurface_Branch, TestSize.Level1)
{
    EXPECT_EQ(processor_->sideSurface_, nullptr);

    SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.timestamp = 1000;

    processor_->CopyToSideSurface(bufferInfo);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OutputBuffer_001
 * @tc.desc: test OutputBuffer when callback != nullptr (branch: callback exists)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OutputBuffer_WithCallback_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->callback_ = callback_;
    EXPECT_NE(processor_->callback_, nullptr);

    callback_->Reset();
    EXPECT_EQ(callback_->outputBufferCount, 0);

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestOutput");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);

    BufferRequestConfig requestCfg;
    requestCfg.width = 100;
    requestCfg.height = 100;
    requestCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    requestCfg.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    GSError err = producerSurface->RequestBuffer(surfaceBuffer, fence, requestCfg);
    if (err == GSERROR_OK && surfaceBuffer != nullptr) {
        SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = surfaceBuffer;
        bufferInfo.timestamp = 1000;

        processor_->OutputBuffer(bufferInfo);
        EXPECT_EQ(callback_->outputBufferCount, 1);
    }
}

/**
 * @tc.name: SideOutputSurfaceProcessor_OutputBuffer_002
 * @tc.desc: test OutputBuffer when callback == nullptr (branch: no callback)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, OutputBuffer_WithoutCallback_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->callback_ = nullptr;

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestOutputNoCb");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);

    BufferRequestConfig requestCfg;
    requestCfg.width = 100;
    requestCfg.height = 100;
    requestCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    requestCfg.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    GSError err = producerSurface->RequestBuffer(surfaceBuffer, fence, requestCfg);
    if (err == GSERROR_OK && surfaceBuffer != nullptr) {
        SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = surfaceBuffer;
        bufferInfo.timestamp = 1000;

        callback_->Reset();
        processor_->OutputBuffer(bufferInfo);
        EXPECT_EQ(callback_->outputBufferCount, 0);
    }
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RenderOutputBufferInner_001
 * @tc.desc: test RenderOutputBufferInner when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferInner_NotRunning_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    Status ret = processor_->RenderOutputBufferInner(0, -1, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RenderOutputBufferInner_002
 * @tc.desc: test RenderOutputBufferInner when buffer not found (branch: invalid index)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferInner_InvalidIndex_Branch, TestSize.Level1)
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
 * @tc.desc: test RenderOutputBufferInner when render == false (branch: not render)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferInner_NotRender_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestNotRender");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);

    BufferRequestConfig requestCfg;
    requestCfg.width = 100;
    requestCfg.height = 100;
    requestCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    requestCfg.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    GSError err = producerSurface->RequestBuffer(surfaceBuffer, fence, requestCfg);
    if (err == GSERROR_OK && surfaceBuffer != nullptr) {
        SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = surfaceBuffer;
        bufferInfo.timestamp = 1000;
        processor_->renderBufferQueue_[surfaceBuffer->GetSeqNum()] = bufferInfo;

        ret = processor_->RenderOutputBufferInner(surfaceBuffer->GetSeqNum(), -1, false);
        EXPECT_EQ(ret, Status::OK);
    }
}

HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNotRenderBuffer_NotIdle_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->state_.load(), SideOutputSurfaceProcessor::ProcessorState::IDLE);

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestDealNotIdle");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);

    BufferRequestConfig requestCfg;
    requestCfg.width = 100;
    requestCfg.height = 100;
    requestCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    requestCfg.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    GSError err = producerSurface->RequestBuffer(surfaceBuffer, fence, requestCfg);
    if (err == GSERROR_OK && surfaceBuffer != nullptr) {
        SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = surfaceBuffer;
        bufferInfo.timestamp = 1000;

        processor_->DealNotRenderBuffer(bufferInfo);
    }
}

HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNotRenderBuffer_Idle_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    processor_->state_ = SideOutputSurfaceProcessor::ProcessorState::IDLE;

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestDealIdle");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);

    BufferRequestConfig requestCfg;
    requestCfg.width = 100;
    requestCfg.height = 100;
    requestCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    requestCfg.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    GSError err = producerSurface->RequestBuffer(surfaceBuffer, fence, requestCfg);
    if (err == GSERROR_OK && surfaceBuffer != nullptr) {
        SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = surfaceBuffer;
        bufferInfo.timestamp = 1000;

        processor_->DealNotRenderBuffer(bufferInfo);
    }
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffers_001
 * @tc.desc: test ProcessBuffers when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffers_NotRunning_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    processor_->ProcessBuffers();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffers_002
 * @tc.desc: test ProcessBuffers when consumerSurface_ == nullptr (branch: null consumer)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffers_NullConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = nullptr;

    processor_->ProcessBuffers();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ProcessBuffers_003
 * @tc.desc: test ProcessBuffers when producerSurface_ == nullptr (branch: null producer)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ProcessBuffers_NullProducerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->producerSurface_ = nullptr;

    processor_->ProcessBuffers();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_WaitTrigger_001
 * @tc.desc: test WaitTrigger when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, WaitTrigger_NotRunning_Branch, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    bool ret = processor_->WaitTrigger();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ClearBufferQueues_001
 * @tc.desc: test ClearBufferQueues when consumerSurface != nullptr (branch: with surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ClearBufferQueues_WithConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> inputSurface = processor_->GetInputSurface();
    EXPECT_NE(inputSurface, nullptr);
    EXPECT_NE(processor_->consumerSurface_, nullptr);

    processor_->ClearBufferQueues();
    EXPECT_TRUE(processor_->consumerBufferQueue_.empty());
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ClearBufferQueues_002
 * @tc.desc: test ClearBufferQueues when consumerSurface == nullptr (branch: without surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ClearBufferQueues_WithoutConsumerSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->consumerSurface_ = nullptr;

    processor_->ClearBufferQueues();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetVideoSample_001
 * @tc.desc: test GetVideoSample when sampleProcessing_ == true (branch: already processing)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_AlreadyProcessing_Branch, TestSize.Level1)
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
 * @tc.desc: test GetVideoSample when sampleProcessing_ == false (branch: not processing)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_NotProcessing_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    EXPECT_FALSE(processor_->sampleProcessing_.load());

    int32_t result = -1;
    processor_->GetVideoSample(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetVideoSample_003
 * @tc.desc: test GetVideoSample when sampleTask_ != nullptr (branch: task exists)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_WithTask_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(processor_->sampleTask_, nullptr);

    int32_t result = -1;
    processor_->GetVideoSample(result);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_GetVideoSample_004
 * @tc.desc: test GetVideoSample when sampleTask_ == nullptr (branch: no task)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, GetVideoSample_WithoutTask_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sampleTask_ = nullptr;

    int32_t result = -1;
    processor_->GetVideoSample(result);
    EXPECT_EQ(result, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_001
 * @tc.desc: test HandleSampleTask when sideBufferQueue_ empty (branch: empty queue)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_EmptyQueue_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sideBufferQueue_.clear();

    processor_->HandleSampleTask();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_002
 * @tc.desc: test HandleSampleTask when !hasData (branch: no data)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_NoData_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
    bufferInfo.buffer = nullptr;
    processor_->sideBufferQueue_.push_back(bufferInfo);

    processor_->HandleSampleTask();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_003
 * @tc.desc: test HandleSampleTask when sideSurface_ == nullptr (branch: null side surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_NullSideSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("TestHandle");
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);

    BufferRequestConfig requestCfg;
    requestCfg.width = 100;
    requestCfg.height = 100;
    requestCfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE;
    requestCfg.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer;
    sptr<SyncFence> fence;
    GSError err = producerSurface->RequestBuffer(surfaceBuffer, fence, requestCfg);
    if (err == GSERROR_OK && surfaceBuffer != nullptr) {
        SideOutputSurfaceProcessor::SurfaceBufferInfo bufferInfo;
        bufferInfo.buffer = surfaceBuffer;
        bufferInfo.timestamp = 1000;
        processor_->sideBufferQueue_.push_back(bufferInfo);

        processor_->sideSurface_ = nullptr;
        processor_->HandleSampleTask();
    }
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_004
 * @tc.desc: test HandleSampleTask when sideSurface_ != nullptr (branch: valid side surface)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_ValidSideSurface_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->sideSurface_ = Surface::CreateSurfaceAsConsumer("TestHandleSide");

    processor_->HandleSampleTask();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_HandleSampleTask_005
 * @tc.desc: test HandleSampleTask when sampleResultPtr_ != nullptr (branch: result ptr exists)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, HandleSampleTask_WithResultPtr_Branch, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    int32_t result = -1;
    processor_->sampleResultPtr_ = &result;

    processor_->HandleSampleTask();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_DealNoImage_001
 * @tc.desc: test DealNoImage when sampleResultPtr_ != nullptr (branch: result ptr exists)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNoImage_WithResultPtr_Branch, TestSize.Level1)
{
    int32_t result = -1;
    processor_->sampleResultPtr_ = &result;

    processor_->DealNoImage();
    EXPECT_EQ(result, 1);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_DealNoImage_002
 * @tc.desc: test DealNoImage when sampleResultPtr_ == nullptr (branch: no result ptr)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, DealNoImage_WithoutResultPtr_Branch, TestSize.Level1)
{
    processor_->sampleResultPtr_ = nullptr;

    processor_->DealNoImage();
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetCallback_001
 * @tc.desc: test SetCallback normal branch
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
 * @tc.desc: test SetEventReceiver normal branch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetEventReceiver_Normal, TestSize.Level1)
{
    Status ret = processor_->SetEventReceiver(receiver_);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(processor_->eventReceiver_, receiver_);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetParameter_001
 * @tc.desc: test SetParameter normal branch
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
 * @tc.desc: test SetPostProcessorOn normal branch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetPostProcessorOn_Normal, TestSize.Level1)
{
    Status ret = processor_->SetPostProcessorOn(true);
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->SetPostProcessorOn(false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetVideoWindowSize_001
 * @tc.desc: test SetVideoWindowSize normal branch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetVideoWindowSize_Normal, TestSize.Level1)
{
    Status ret = processor_->SetVideoWindowSize(1920, 1080);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_NotifyEos_001
 * @tc.desc: test NotifyEos normal branch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, NotifyEos_Normal, TestSize.Level1)
{
    Status ret = processor_->NotifyEos(1000);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_SetVideoOutput_001
 * @tc.desc: test SetVideoOutput normal branch
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, SetVideoOutput_Normal, TestSize.Level1)
{
    sptr<Surface> sideSurface = Surface::CreateSurfaceAsConsumer("TestVideoOutput");
    processor_->SetVideoOutput(sideSurface);
    EXPECT_EQ(processor_->sideSurface_, sideSurface);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_ReleaseOutputBuffer_001
 * @tc.desc: test ReleaseOutputBuffer when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, ReleaseOutputBuffer_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    Status ret = processor_->ReleaseOutputBuffer(0, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_RenderOutputBufferAtTime_001
 * @tc.desc: test RenderOutputBufferAtTime when !isRunning_ (branch: not running)
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, RenderOutputBufferAtTime_NotRunning, TestSize.Level1)
{
    EXPECT_FALSE(processor_->isRunning_.load());

    Status ret = processor_->RenderOutputBufferAtTime(0, 1000);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SideOutputSurfaceProcessor_FullLifecycle_001
 * @tc.desc: test full lifecycle with all methods
 * @tc.type: FUNC
 */
HWTEST_F(SideOutputSurfaceProcessorUnitTest, FullLifecycle, TestSize.Level1)
{
    Status ret = processor_->Init();
    EXPECT_EQ(ret, Status::OK);

    processor_->SetCallback(callback_);
    processor_->SetEventReceiver(receiver_);

    sptr<Surface> inputSurface = processor_->GetInputSurface();
    EXPECT_NE(inputSurface, nullptr);

    ret = processor_->Start();
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->Flush();
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->Stop();
    EXPECT_EQ(ret, Status::OK);

    ret = processor_->Release();
    EXPECT_EQ(ret, Status::OK);
    processor_ = nullptr;
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS