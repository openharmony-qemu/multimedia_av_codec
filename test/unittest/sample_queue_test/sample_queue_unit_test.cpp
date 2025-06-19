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

#include "sample_queue_unit_test.h"

#define LOCAL true
namespace OHOS::Media {
using namespace testing::ext;
constexpr int64_t FRAME_INTERVAL_MS = 5;

void SampleQueueUnitTest::SetUpTestCase(void)
{}

void SampleQueueUnitTest::TearDownTestCase(void)
{}

void SampleQueueUnitTest::SetUp()
{
    sampleQueue_ = std::make_shared<SampleQueue>();
    sqCb_ = std::make_shared<SampleQueueCallbackMock>();
    sampleQueue_->SetSampleQueueCallback(sqCb_);
}

void SampleQueueUnitTest::TearDown()
{}

Status SampleQueueUnitTest::InitLargeSampleQueue()
{
    SampleQueue::Config sampleQueueConfig{};
    sampleQueueConfig.isFlvLiveStream_ = true;
    sampleQueueConfig.isSupportBitrateSwitch_ = true;
    sampleQueueConfig.queueId_ = 1;
    sampleQueueConfig.bufferCap_ = SampleQueue::MAX_SAMPLE_QUEUE_SIZE;
    Status status = sampleQueue_->Init(sampleQueueConfig);
    return status;
}

Status SampleQueueUnitTest::InitNormalSampleQueue()
{
    SampleQueue::Config sampleQueueConfig{};
    sampleQueueConfig.isFlvLiveStream_ = false;
    sampleQueueConfig.isSupportBitrateSwitch_ = false;
    sampleQueueConfig.queueId_ = 0;
    sampleQueueConfig.bufferCap_ = 1;
    Status status = sampleQueue_->Init(sampleQueueConfig);
    EXPECT_EQ(status, Status::OK);
    return status;
}

Status SampleQueueUnitTest::UpdateBufferInfo(
    const std::shared_ptr<AVBuffer> &buffer, int64_t pts, size_t bufferSize, bool isKeyFrame)
{
    EXPECT_EQ(buffer->memory_ != nullptr, true);
    buffer->memory_->SetSize(bufferSize);
    EXPECT_EQ(buffer->GetConfig().size, bufferSize);
    buffer->pts_ = pts;
    if (isKeyFrame) {
        buffer->flag_ |= static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    }
    return Status::OK;
}

void SampleQueueUnitTest::ProducerLoop(int64_t frameCount, int64_t frameIntervalMs, size_t bufferSize)
{
    int64_t pushFrames = 0;
    while (pushFrames < frameCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(frameIntervalMs));
        int64_t pts = pushFrames;
        AVBufferConfig avBufferConfig;
        avBufferConfig.capacity = bufferSize;
        avBufferConfig.size = bufferSize;
        std::shared_ptr<AVBuffer> sampleBuffer;
        Status status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
        if (status != Status::OK) {
            continue;
        }

        UpdateBufferInfo(sampleBuffer, pts, bufferSize);

        status = sampleQueue_->PushBuffer(sampleBuffer, true);
        pushFrames++;
        std::cout << "========== pushFrames " << pushFrames << std::endl;
    }
}

void SampleQueueUnitTest::ConsumerLoop(int64_t frameCount, int64_t frameIntervalMs)
{
    int64_t acquireFrames = 0;
    while (acquireFrames < frameCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(frameIntervalMs));

        size_t size = 0;
        Status status = sampleQueue_->QuerySizeForNextAcquireBuffer(size);
        if (status != Status::OK) {
            continue;
        }
        AVBufferConfig avBufferConfig;
        avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
        avBufferConfig.capacity = size;
        avBufferConfig.size = size;
        auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
        std::shared_ptr<AVBuffer> dstBuffer = AVBuffer::CreateAVBuffer(avAllocator, avBufferConfig.capacity);
        status = sampleQueue_->AcquireCopyToDstBuffer(dstBuffer);
        if (status != Status::OK) {
            continue;
        }
        acquireFrames++;
        std::cout << "========== acquireFrames " << acquireFrames << std::endl;
    }
}

/**
 * @tc.name: SampleQueue_GetBufferQueueProducer
 * @tc.desc: test SampleQueue Get BufferQueueProducer
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_GetBufferQueueProducer, TestSize.Level1)
{
    EXPECT_EQ(InitNormalSampleQueue(), Status::OK);
    EXPECT_TRUE(sampleQueue_->GetBufferQueueProducer() != nullptr);
}

/**
 * @tc.name: SampleQueue_Init_Not_Support_Bitrate_Switch
 * @tc.desc: test SampleQueue init but not support bitrate switch
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_Init_Not_Support_Bitrate_Switch, TestSize.Level1)
{
    EXPECT_EQ(InitNormalSampleQueue(), Status::OK);
}

/**
 * @tc.name: SampleQueue_Init_Support_Bitrate_Switch
 * @tc.desc: test SampleQueue init and support bitrate switch
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_Init_Support_Bitrate_Switch, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
}

/**
 * @tc.name: SampleQueue_RequestBuffer
 * @tc.desc: test SampleQueue request Buffer
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_RequestBuffer, TestSize.Level1)
{
    Status status = InitLargeSampleQueue();
    EXPECT_EQ(status, Status::OK);
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = 1024;
    std::shared_ptr<AVBuffer> sampleBuffer;
    status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
    EXPECT_EQ(status, Status::OK);
}

/**
 * @tc.name: SampleQueue_PushBuffer
 * @tc.desc: test SampleQueue push Buffer
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_PushBuffer, TestSize.Level1)
{
    Status status = InitLargeSampleQueue();
    EXPECT_EQ(status, Status::OK);

    AVBufferConfig avBufferConfig;
    avBufferConfig.size = 512;
    avBufferConfig.capacity = 1024;
    std::shared_ptr<AVBuffer> sampleBuffer;
    status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
    EXPECT_EQ(status, Status::OK);

    UpdateBufferInfo(sampleBuffer, 1, 512);

    status = sampleQueue_->PushBuffer(sampleBuffer, true);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(sqCb_->OnConsumeSum_, 1);
}

/**
 * @tc.name: SampleQueue_AcquireCopyToDstBuffer
 * @tc.desc: test SampleQueue Acquire and Copy to Dst Buffer
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_AcquireCopyToDstBuffer, TestSize.Level1)
{
    Status status = InitLargeSampleQueue();
    EXPECT_EQ(status, Status::OK);

    constexpr size_t bufferSize = 1024;
    constexpr int64_t pts = 1;
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = bufferSize;
    avBufferConfig.size = bufferSize;
    std::shared_ptr<AVBuffer> sampleBuffer;
    status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
    EXPECT_EQ(status, Status::OK);

    UpdateBufferInfo(sampleBuffer, pts, bufferSize);

    status = sampleQueue_->PushBuffer(sampleBuffer, true);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(sqCb_->OnConsumeSum_, 1);

    size_t size = 0;
    status = sampleQueue_->QuerySizeForNextAcquireBuffer(size);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(size, bufferSize);

    avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    avBufferConfig.capacity = 2048;
    std::shared_ptr<AVBuffer> sampleBuffer2 = AVBuffer::CreateAVBuffer(avBufferConfig);
    status = sampleQueue_->AcquireCopyToDstBuffer(sampleBuffer2);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(sampleBuffer2->pts_, pts);
}

/**
 * @tc.name: SampleQueue_AcquireCopyToDstBufferFail
 * @tc.desc: test SampleQueue Acquire and Copy to Dst Buffer Fail
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_AcquireCopyToDstBufferFail, TestSize.Level1)
{
    Status status = InitLargeSampleQueue();
    EXPECT_EQ(status, Status::OK);

    constexpr size_t srcBufferSize = 1024;
    constexpr size_t dstBufferSize = 512;
    constexpr int64_t pts = 1;
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = srcBufferSize;
    avBufferConfig.size = srcBufferSize;
    std::shared_ptr<AVBuffer> sampleBuffer;
    status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
    EXPECT_EQ(status, Status::OK);

    UpdateBufferInfo(sampleBuffer, pts, srcBufferSize);

    status = sampleQueue_->PushBuffer(sampleBuffer, true);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(sqCb_->OnConsumeSum_, 1);

    size_t size = 0;
    status = sampleQueue_->QuerySizeForNextAcquireBuffer(size);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(size, srcBufferSize);

    avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    avBufferConfig.capacity = dstBufferSize;
    auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
    std::shared_ptr<AVBuffer> sampleBuffer2 = AVBuffer::CreateAVBuffer(avAllocator, avBufferConfig.capacity);
    status = sampleQueue_->AcquireCopyToDstBuffer(sampleBuffer2);
    EXPECT_NE(status, Status::OK);
}

/**
 * @tc.name: SampleQueue_AcquireCopyToDstBufferFail
 * @tc.desc: test SampleQueue Acquire and Copy to Dst Buffer Fail
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_AcquireCopyToDstBufferFailForNullMeta, TestSize.Level1)
{
    Status status = InitLargeSampleQueue();
    EXPECT_EQ(status, Status::OK);

    constexpr size_t srcBufferSize = 1024;
    constexpr size_t dstBufferSize = 512;
    constexpr int64_t pts = 1;
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = srcBufferSize;
    avBufferConfig.size = srcBufferSize;
    std::shared_ptr<AVBuffer> srcBuffer;
    status = sampleQueue_->RequestBuffer(srcBuffer, avBufferConfig, 0);
    EXPECT_EQ(status, Status::OK);

    UpdateBufferInfo(srcBuffer, pts, srcBufferSize);
    EXPECT_TRUE(srcBuffer->meta_ != nullptr);

    status = sampleQueue_->PushBuffer(srcBuffer, true);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(sqCb_->OnConsumeSum_, 1);

    size_t size = 0;
    status = sampleQueue_->QuerySizeForNextAcquireBuffer(size);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(size, srcBufferSize);

    avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    avBufferConfig.capacity = dstBufferSize;
    auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
    std::shared_ptr<AVBuffer> dstBuffer = AVBuffer::CreateAVBuffer(avAllocator, avBufferConfig.capacity);

    srcBuffer->meta_ = nullptr;
    status = sampleQueue_->AcquireCopyToDstBuffer(dstBuffer);
    EXPECT_NE(status, Status::OK);
}

/**
 * @tc.name: SampleQueue_AcquireCopyToDstBufferFailForNullMemory
 * @tc.desc: test SampleQueue Acquire and Copy to Dst Buffer Fail for nullptr memory
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_AcquireCopyToDstBufferFailForNullMemory, TestSize.Level1)
{
    Status status = InitLargeSampleQueue();
    EXPECT_EQ(status, Status::OK);

    constexpr size_t srcBufferSize = 1024;
    constexpr size_t dstBufferSize = 512;
    constexpr int64_t pts = 1;
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = srcBufferSize;
    avBufferConfig.size = srcBufferSize;
    std::shared_ptr<AVBuffer> srcBuffer;
    status = sampleQueue_->RequestBuffer(srcBuffer, avBufferConfig, 0);
    EXPECT_EQ(status, Status::OK);

    UpdateBufferInfo(srcBuffer, pts, srcBufferSize);
    EXPECT_TRUE(srcBuffer->memory_ != nullptr);

    status = sampleQueue_->PushBuffer(srcBuffer, true);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(sqCb_->OnConsumeSum_, 1);

    size_t size = 0;
    status = sampleQueue_->QuerySizeForNextAcquireBuffer(size);
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(size, srcBufferSize);

    avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    avBufferConfig.capacity = dstBufferSize;
    auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
    std::shared_ptr<AVBuffer> dstBuffer = AVBuffer::CreateAVBuffer(avAllocator, avBufferConfig.capacity);

    dstBuffer->memory_ = nullptr;
    status = sampleQueue_->AcquireCopyToDstBuffer(dstBuffer);
    EXPECT_NE(status, Status::OK);
}

/**
 * @tc.name: SampleQueue_NormalSampleQueueLoop
 * @tc.desc: test SampleQueue for normal sample queue loop
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_NormalSampleQueueLoop, TestSize.Level1)
{
    EXPECT_EQ(InitNormalSampleQueue(), Status::OK);

    int64_t frameCount = 100;
    int64_t frameIntervalMs = 1;
    size_t bufferSize = 512;

    // threadProducer: push and producer
    auto producerThread = std::make_shared<std::thread>(
        &SampleQueueUnitTest::ProducerLoop, this, frameCount, frameIntervalMs, bufferSize);

    // threadConsumer: acquire and comsumer
    auto consumerThread =
        std::make_shared<std::thread>(&SampleQueueUnitTest::ConsumerLoop, this, frameCount, frameIntervalMs);
    producerThread.get()->join();
    consumerThread.get()->join();

    EXPECT_EQ(sqCb_->OnConsumeSum_, frameCount);
    EXPECT_EQ(sqCb_->OnAvailableSum_, frameCount + SampleQueue::DEFAULT_SAMPLE_QUEUE_SIZE);
}

/**
 * @tc.name: SampleQueue_LargerSampleQueueLoop
 * @tc.desc: test SampleQueue for switch bitrate sample queue loop
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_LargerSampleQueueLoop, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);

    int64_t frameCount = 200;
    int64_t frameIntervalMs = 1;
    size_t bufferSize = 512;

    // threadProducer: push and producer
    auto producerThread = std::make_shared<std::thread>(
        &SampleQueueUnitTest::ProducerLoop, this, frameCount, frameIntervalMs, bufferSize);

    // threadConsumer: acquire and comsumer
    auto consumerThread =
        std::make_shared<std::thread>(&SampleQueueUnitTest::ConsumerLoop, this, frameCount, frameIntervalMs);
    producerThread.get()->join();
    consumerThread.get()->join();

    EXPECT_EQ(sqCb_->OnConsumeSum_, frameCount);
    EXPECT_EQ(sqCb_->OnAvailableSum_, frameCount + SampleQueue::MAX_SAMPLE_QUEUE_SIZE);
}

/**
 * @tc.name: SampleQueue_SwitchBitrate
 * @tc.desc: test SampleQueue for switch bitrate sample queue loop
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_SwitchBitrateNormal, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);

    int64_t frameCount = 500;
    int64_t selectBitrateFrameCount = 300;
    int64_t frameIntervalMs = 33;
    size_t bufferSize = 512;

    // threadProducer: push and producer
    producerThread_ = std::make_shared<std::thread>([this, &frameCount, &frameIntervalMs, &bufferSize]() {
        while (pushFrames_ < frameCount) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            int64_t pts = pushFrames_ * FRAME_INTERVAL_MS * 1000;
            AVBufferConfig avBufferConfig;
            avBufferConfig.capacity = bufferSize;
            avBufferConfig.size = bufferSize;
            std::shared_ptr<AVBuffer> sampleBuffer;
            Status status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
            if (status != Status::OK) {
                continue;
            }
            bool isKeyFrame = pushFrames_ % 30 == 0;
            UpdateBufferInfo(sampleBuffer, pts, bufferSize, isKeyFrame);

            status = sampleQueue_->PushBuffer(sampleBuffer, true);
            pushFrames_++;
            std::cout << "========== pushFrames " << pushFrames_ << std::endl;
            if (pushFrames_ == 300) {
                sampleQueue_->ReadySwitchBitrate(2000);
            }
            if (sqCb_->switchPtsVec_.size() > 0) {
                break;
            }
        }
    });

    // threadConsumer: acquire and comsumer
    consumerThread_ = std::make_shared<std::thread>(
        &SampleQueueUnitTest::ConsumerLoop, this, selectBitrateFrameCount, frameIntervalMs);
    producerThread_.get()->join();
    consumerThread_.get()->join();
    EXPECT_EQ(sqCb_->switchPtsVec_.size(), 1);
}

/**
 * @tc.name: SampleQueue_DiscardSampleAfter
 * @tc.desc: test SampleQueue for discard sample
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_DiscardSampleAfter, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
    int64_t frameCount = SampleQueue::MAX_SAMPLE_QUEUE_SIZE;
    int64_t frameIntervalMs = 1;
    size_t bufferSize = 512;
    int64_t pushFrames = 0;
    while (pushFrames < frameCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(frameIntervalMs));
        int64_t pts = pushFrames;
        AVBufferConfig avBufferConfig;
        avBufferConfig.capacity = bufferSize;
        avBufferConfig.size = bufferSize;
        std::shared_ptr<AVBuffer> sampleBuffer;
        Status status = sampleQueue_->RequestBuffer(sampleBuffer, avBufferConfig, 0);
        if (status != Status::OK) {
            continue;
        }
        bool isKeyFrame = pushFrames_ % 30 == 0;
        UpdateBufferInfo(sampleBuffer, pts, bufferSize);
        if (isKeyFrame) {
            sampleQueue_->keyFramePtsSet_.insert(pts);
        }

        status = sampleQueue_->PushBuffer(sampleBuffer, true);
        pushFrames++;
        std::cout << "========== pushFrames " << pushFrames << std::endl;
    }
    EXPECT_EQ(sampleQueue_->DiscardSampleAfter(200), Status::OK);
    EXPECT_EQ(sampleQueue_->lastEndSamplePts_, 200);
}

/**
 * @tc.name: SampleQueue_ReadySwitchBitrateInvalid
 * @tc.desc: test SampleQueue for ReadySwitchBitrate Invalid
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_ReadySwitchBitrateInvalid, TestSize.Level1)
{
    EXPECT_EQ(InitNormalSampleQueue(), Status::OK);
    EXPECT_EQ(sampleQueue_->ReadySwitchBitrate(2000), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: SampleQueue_ReadySwitchBitrateFromReadySwitch
 * @tc.desc: test SampleQueue for SampleQueue_ReadySwitchBitrateFromReadySwitch
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_ReadySwitchBitrateFromReadySwitch, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
    uint32_t bitrate = 2000;
    sampleQueue_->switchStatus_ = SelectBitrateStatus::READY_SWITCH;
    EXPECT_EQ(sampleQueue_->ReadySwitchBitrate(bitrate), Status::OK);
    EXPECT_EQ(sampleQueue_->nextSwitchBitrate_, bitrate);
}

/**
 * @tc.name: SampleQueue_ReadySwitchBitrateFromSwitching
 * @tc.desc: test SampleQueue for SampleQueue_ReadySwitchBitrateFromSwitching
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_ReadySwitchBitrateFromSwitching, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
    uint32_t bitrate = 2000;
    sampleQueue_->switchStatus_ = SelectBitrateStatus::SWITCHING;
    EXPECT_EQ(sampleQueue_->ReadySwitchBitrate(bitrate), Status::OK);
    EXPECT_EQ(sampleQueue_->switchBitrateWaitList_.front(), bitrate);
}

/**
 * @tc.name: SampleQueue_ReadySwitchBitrateFromSwitchingWithWaitList
 * @tc.desc: test SampleQueue for SampleQueue_ReadySwitchBitrateFromSwitchingWithWaitList
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_ReadySwitchBitrateFromSwitchingWithWaitList, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
    uint32_t bitrate = 1000;
    sampleQueue_->switchBitrateWaitList_.push_back(bitrate);
    bitrate = 2000;
    sampleQueue_->switchStatus_ = SelectBitrateStatus::SWITCHING;
    EXPECT_EQ(sampleQueue_->ReadySwitchBitrate(bitrate), Status::OK);
    EXPECT_EQ(sampleQueue_->switchBitrateWaitList_.front(), bitrate);
    EXPECT_EQ(sampleQueue_->switchBitrateWaitList_.size(), SampleQueue::MAX_BITRATE_SWITCH_WAIT_NUMBER);
}

/**
 * @tc.name: SampleQueue_IsKeyFrameAvailable
 * @tc.desc: test SampleQueue for IsKeyFrameAvailable
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_IsKeyFrameAvailable, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
    sampleQueue_->lastEndSamplePts_ = 2500000;
    sampleQueue_->keyFramePtsSet_.insert(3000000);
    sampleQueue_->keyFramePtsSet_.insert(4000000);
    EXPECT_TRUE(sampleQueue_->IsKeyFrameAvailable());
    EXPECT_EQ(sampleQueue_->startPtsToSwitch_, 4000000);

    sampleQueue_->keyFramePtsSet_.insert(5000000);
    sampleQueue_->keyFramePtsSet_.insert(6000000);
    sampleQueue_->keyFramePtsSet_.insert(7000000);
    EXPECT_TRUE(sampleQueue_->IsKeyFrameAvailable());
    EXPECT_EQ(sampleQueue_->startPtsToSwitch_, 6000000);

    sampleQueue_->keyFramePtsSet_.clear();
    EXPECT_FALSE(sampleQueue_->IsKeyFrameAvailable());
}

/**
 * @tc.name: SampleQueue_GetCacheDuration
 * @tc.desc: test SampleQueue for GetCacheDuration
 * @tc.type: FUNC
 */

HWTEST_F(SampleQueueUnitTest, SampleQueue_GetCacheDuration, TestSize.Level1)
{
    EXPECT_EQ(InitLargeSampleQueue(), Status::OK);
    sampleQueue_->lastEnterSamplePts_ = Plugins::HST_TIME_NONE;
    EXPECT_EQ(sampleQueue_->GetCacheDuration(), 0);

    sampleQueue_->lastOutSamplePts_ = Plugins::HST_TIME_NONE;
    EXPECT_EQ(sampleQueue_->GetCacheDuration(), 0);

    sampleQueue_->lastOutSamplePts_ = 1000;
    sampleQueue_->lastEnterSamplePts_ = 3000;
    EXPECT_EQ(sampleQueue_->GetCacheDuration(), 2000);
}
}  // namespace OHOS::Media
