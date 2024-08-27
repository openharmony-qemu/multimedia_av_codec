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

#include "video_resize_filter_unit_test.h"
#include "video_resize_filter.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"

#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "detail_enhancer_video.h"
#include "detail_enhancer_video_common.h"
#endif

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
#ifdef USE_VIDEO_PROCESSING_ENGINE
using namespace VideoProcessingEngine;
#endif
namespace Pipeline {
void VideoResizeFilterUnitTest::SetUpTestCase(void) {}

void VideoResizeFilterUnitTest::TearDownTestCase(void) {}

void VideoResizeFilterUnitTest::SetUp(void)
{
    videoResize_ = std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter",
                        Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
}

void VideoResizeFilterUnitTest::TearDown(void)
{
    videoResize_ = nullptr;
}

/**
 * @tc.name: VideoResizeFilter_Init_001
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_Init_001, TestSize.Level1)
{
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    videoResize_->releaseBufferTask_ = nullptr;
    videoResize_->Init(eventReceive, filterCallback);
    videoResize_->releaseBufferTask_ = std::make_shared<Task>("VideoResize");
    videoResize_->Init(eventReceive, filterCallback);
    videoResize_->videoEnhancer_ = nullptr;
    videoResize_->Init(eventReceive, filterCallback);
    ASSERT_EQ(videoResize_->appUid_, 0);
}

/**
 * @tc.name: VideoResizeFilter_Configure_001
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_Configure_001, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter= std::make_shared<Meta>();
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize_->videoEnhancer_ = nullptr;
    Status ret = videoResize_->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    ret = videoResize_->Configure(parameter);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoResizeFilter_GetInputSurface_001
 * @tc.desc: GetInputSurface
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_GetInputSurface_001, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter= std::make_shared<Meta>();
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize_->videoEnhancer_ = nullptr;
    sptr<Surface> inputsurface = videoResize_->GetInputSurface();
    EXPECT_EQ(inputsurface, nullptr);
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    inputsurface = videoResize_->GetInputSurface();
    EXPECT_NE(inputsurface, nullptr);
}

/**
 * @tc.name: VideoResizeFilter_SetOutputSurface_001
 * @tc.desc: SetOutputSurface
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_SetOutputSurface_001, TestSize.Level1)
{
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    sptr<Surface> surface = nullptr;
    Status ret = videoResize_->SetOutputSurface(surface, 0, 0);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    videoResize_->eventReceiver_ = nullptr;
    surface = videoResize_->videoEnhancer_->GetInputSurface();
    ret = videoResize_->SetOutputSurface(surface, 0, 0);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: VideoResizeFilter_DoStart_001
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_DoStart_001, TestSize.Level1)
{
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize_->releaseBufferTask_ = std::make_shared<Task>("test");
    videoResize_->videoEnhancer_ = nullptr;
    Status ret = videoResize_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
    videoResize_->releaseBufferTask_ = nullptr;
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    videoResize_->eventReceiver_ = nullptr;
    ret = videoResize_->DoStart();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: VideoResizeFilter_DoStop_001
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_DoStop_001, TestSize.Level1)
{
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize_->releaseBufferTask_ = nullptr;
    videoResize_->videoEnhancer_ = nullptr;
    Status ret = videoResize_->DoStop();
    EXPECT_EQ(ret, Status::OK);
    videoResize_->releaseBufferTask_ = std::make_shared<Task>("test");
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    videoResize_->eventReceiver_ = nullptr;
    ret = videoResize_->DoStop();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: VideoResizeFilter_SetParameter_001
 * @tc.desc: SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_SetParameter_001, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter= std::make_shared<Meta>();
    videoResize_->videoEnhancer_ = nullptr;
    videoResize_->SetParameter(parameter);
    videoResize_->videoEnhancer_ = DetailEnhancerVideo::Create();
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize_->SetParameter(parameter);
    videoResize_->eventReceiver_ = nullptr;
    EXPECT_EQ(videoResize_->appPid_, 0);
}

/**
 * @tc.name: VideoResizeFilter_LinkNext_001
 * @tc.desc: LinkNext
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_LinkNext_001, TestSize.Level1)
{
    std::shared_ptr<Filter> nextFilter = std::make_shared<TestFilter>();
    videoResize_->configureParameter_ = std::make_shared<Meta>();
    videoResize_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    StreamType outType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status ret = videoResize_->LinkNext(nextFilter, outType);
    videoResize_->eventReceiver_ = nullptr;
    ret = videoResize_->LinkNext(nextFilter, outType);
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name: VideoResizeFilter_ReleaseBuffer_001
 * @tc.desc: ReleaseBuffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_ReleaseBuffer_001, TestSize.Level1)
{
    videoResize_->isThreadExit_ = true;
    videoResize_->ReleaseBuffer();
    EXPECT_EQ(videoResize_->instanceId_, 0);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS