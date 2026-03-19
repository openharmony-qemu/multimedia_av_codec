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
 * @tc.name: VideoCaptureFilter_SetInputSurface_001
 * @tc.desc: SetInputSurface
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
 * @tc.name: VideoCaptureFilter_DoStop_001
 * @tc.desc: DoStop
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
 * @tc.desc: OnBufferAvailable whole process
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnBufferAvailable_001, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    videoCaptureFilter_->SetParameter(parameter);
    videoCaptureFilter_->GetParameter(parameter);
    videoCaptureFilter_->OnUnlinkedResult(parameter);
    videoCaptureFilter_->OnUpdatedResult(parameter);

    sptr<MockConsumerSurface> mockConsumerSurface = MockConsumerSurface::CreateSurfaceAsConsumer("MockConsumerSurface");
    Status ret = videoCaptureFilter_->SetInputSurface(mockConsumerSurface);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(mockConsumerSurface->producer_, nullptr);
    EXPECT_NE(mockConsumerSurface->consumer_, nullptr);
    EXPECT_NE(videoCaptureFilter_->inputSurface_, nullptr);

    videoCaptureFilter_->OnBufferAvailable();
}

HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_SetCodecFormat_001, TestSize.Level1)
{
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    Status ret = videoCaptureFilter_->SetCodecFormat(videoMeta);
    EXPECT_EQ(ret, Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS