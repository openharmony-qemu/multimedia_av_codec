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
    Status ret = videoCaptureFilter_->Configure();
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
    Status ret = videoCaptureFilter_->DoPrepare();
    EXPECT_NE(ret, Status::OK);
    videoCaptureFilter_->filterCallback_ = std::make_shared<TestFilterCallback>();
    ret = videoCaptureFilter_->DoPrepare();
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
    EXPECT_EQ(videoCaptureFilter_->isStop_, true):
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
    Status ret = videoCaptureFilter_->LinkNext();
    EXPECT_EQ(ret, Status::OK); 
    EXPECT_EQ(videoCaptureFilter_->latestBufferTime_, TIME_NONE);
    EXPECT_EQ(videoCaptureFilter_->latestPausedTime_, TIME_NONE);
    EXPECT_EQ(videoCaptureFilter_->totalPausedTime_, 0);
    EXPECT_EQ(videoCaptureFilter_->refreshTotalPauseTime_, false);
}

/**
 * @tc.name: VideoCaptureFilter_DoStop_001
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateNext_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->UpdateNext();
    EXPECT_EQ(ret, Status::OK); 
}

/**
 * @tc.name: VideoCaptureFilter_UnLinkNext_001
 * @tc.desc: UnLinkNext
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UnLinkNext_001, TestSize.Level1)
{
    Status ret = videoCaptureFilter_->UnLinkNext();
    EXPECT_EQ(ret, Status::OK); 
}

/**
 * @tc.name: VideoCaptureFilter_GetFilterType_001
 * @tc.desc: GetFilterType
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_GetFilterType_001, TestSize.Level1)
{
    FilterType filterType_ = videoCaptureFilter_->GetFilterType();
    EXPECT_EQ(ret, FilterType::VIDEO_CAPTURE); 
}

/**
 * @tc.name: VideoCaptureFilter_OnLinked_001
 * @tc.desc: OnLinked
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnLinked_001, TestSize.Level1)
{
    std::shared_ptr<TestFilterCallback> testFilterCallback = std::make_shared<TestFilterCallback>():
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    StreamType inType = Pipeline::StreamType::STREAMTYPE_PACKED
    Status ret = videoCaptureFilter_->OnLinked(inType, videoMeta, testFilterCallback);
    EXPECT_EQ(ret, Status::OK); 
}

/**
 * @tc.name: VideoCaptureFilter_OnUpdated_001
 * @tc.desc: OnUpdated
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnUpdated_001, TestSize.Level1)
{
    std::shared_ptr<TestFilterCallback> testFilterCallback = std::make_shared<TestFilterCallback>():
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    StreamType inType = Pipeline::StreamType::STREAMTYPE_PACKED
    Status ret = videoCaptureFilter_->OnUpdated(inType, videoMeta, testFilterCallback);
    EXPECT_EQ(ret, Status::OK); 
}

/**
 * @tc.name: VideoCaptureFilter_OnLinkedResult_001
 * @tc.desc: OnLinkedResult
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_OnLinkedResult_001, TestSize.Level1)
{
    std::shared_ptr<MyAVBufferQueueProducer> myOutputBufferQueue = std::make_shared<MyAVBufferQueueProducer>();
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoCaptureFilter_->OnLinkedResult(myOutputBufferQueue, videoMeta);
    EXPECT_EQ(videoCaptureFilter_->outputBufferQueueProducer_, myOutputBufferQueue); 
}




/**
 * @tc.name: VideoCaptureFilter_UpdateBufferConfig_001
 * @tc.desc: UpdateBufferConfig
 * @tc.type: FUNC
 */
HWTEST_F(VideoCaptureFilterUnitTest, VideoCaptureFilter_UpdateBufferConfig_001, TestSize.Level1)
{
    FilterType filterType_ = videoCaptureFilter_->UpdateBufferConfig();
    EXPECT_EQ(ret, FilterType::VIDEO_CAPTURE); 
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS