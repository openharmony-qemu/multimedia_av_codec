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

#include "video_resize_filter_unit_test.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"

#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "detail_enhancer_video.h"
#include "detail_enhancer_video_common.h"
#endif

#include "video_resize_filter.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
void VideoResizeFilterUnitTest::SetUpTestCase(void)
{
}

void VideoResizeFilterUnitTest::TearDownTestCase(void)
{
}

void VideoResizeFilterUnitTest::SetUp()
{
}

void VideoResizeFilterUnitTest::TearDown()
{
}

HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> format;
    EXPECT_EQ(videoResize->SetCodecFormat(format), Status::OK);


    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    videoResize->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> parameter;
    EXPECT_EQ(videoResize->Configure(parameter), Status::ERROR_UNKNOWN);

    EXPECT_EQ(videoResize->GetInputSurface(), nullptr);
    EXPECT_EQ(videoResize->SetOutputSurface(nullptr, 1, 1), Status::ERROR_UNKNOWN);

    EXPECT_EQ(videoResize->DoPrepare(), Status::OK);
    EXPECT_EQ(videoResize->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoResize->DoPause(), Status::OK);
    EXPECT_EQ(videoResize->DoResume(), Status::OK);
    EXPECT_EQ(videoResize->DoStop(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoResize->DoFlush(), Status::OK);
    EXPECT_EQ(videoResize->DoRelease(), Status::OK);

    videoResize->GetParameter(parameter);
    EXPECT_EQ(videoResize->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(videoResize->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    videoResize->GetFilterType();

    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	    AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    videoResize->OnLinkedResult(inputBufferQueueProducer, format);
    videoResize->OnUpdatedResult(format);
    videoResize->OnUnlinkedResult(format);
    videoResize->SetFaultEvent("111", 0);
    videoResize->SetFaultEvent("111");
    videoResize->SetCallingInfo(1, 1, "111", 1);

    std::shared_ptr<TestFilterLinkCallback> filterLinkCallback = std::make_shared<TestFilterLinkCallback>();

    EXPECT_EQ(videoResize->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback), Status::OK);
    EXPECT_EQ(videoResize->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback), Status::OK);
    EXPECT_EQ(videoResize->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, filterLinkCallback), Status::OK);
}

HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    videoResize->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::MEDIA_END_OF_STREAM>(true);
    videoResize->OnOutputBufferAvailable(1, 0);
    videoResize->SetParameter(parameter);

    EXPECT_EQ(videoResize->DoPrepare(), Status::OK);
    videoResize->filterCallback_ = nullptr;
    EXPECT_EQ(videoResize->DoPrepare(), Status::ERROR_UNKNOWN);
}

HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_003, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    videoResize->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::MEDIA_END_OF_STREAM>(true);
    videoResize->OnOutputBufferAvailable(1, 1);
    videoResize->SetParameter(parameter);

    EXPECT_EQ(videoResize->DoPrepare(), Status::OK);
    videoResize->filterCallback_ = nullptr;
    EXPECT_EQ(videoResize->DoPrepare(), Status::ERROR_UNKNOWN);
}

HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_004, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> format;
    EXPECT_EQ(videoResize->SetCodecFormat(format), Status::OK);


    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    videoResize->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> parameter;
    EXPECT_EQ(videoResize->Configure(parameter), Status::ERROR_UNKNOWN);

    EXPECT_EQ(videoResize->GetInputSurface(), nullptr);
    EXPECT_EQ(videoResize->SetOutputSurface(nullptr, 1, 1), Status::ERROR_UNKNOWN);

    EXPECT_EQ(videoResize->DoPrepare(), Status::OK);
    EXPECT_EQ(videoResize->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoResize->DoPause(), Status::OK);
    EXPECT_EQ(videoResize->DoResume(), Status::OK);
    EXPECT_EQ(videoResize->DoStop(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoResize->DoFlush(), Status::OK);
    EXPECT_EQ(videoResize->DoRelease(), Status::OK);

    videoResize->GetParameter(parameter);
    EXPECT_EQ(videoResize->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(videoResize->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    videoResize->GetFilterType();

    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	    AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    videoResize->OnUnlinkedResult(format);
    videoResize->OnUpdatedResult(format);
    videoResize->OnLinkedResult(inputBufferQueueProducer, format);
    videoResize->OnUpdatedResult(format);
    videoResize->OnUnlinkedResult(format);
    videoResize->SetFaultEvent("111", 0);
    videoResize->SetFaultEvent("111");
    videoResize->SetCallingInfo(1, 1, "111", 1);

    std::shared_ptr<TestFilterLinkCallback> filterLinkCallback = std::make_shared<TestFilterLinkCallback>();

    EXPECT_EQ(videoResize->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback), Status::OK);
    EXPECT_EQ(videoResize->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback), Status::OK);
    EXPECT_EQ(videoResize->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, filterLinkCallback), Status::OK);
}

HWTEST_F(VideoResizeFilterUnitTest, VideoResizeFilter_005, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    videoResize->Init(eventReceive, filterCallback);

#ifdef USE_VIDEO_PROCESSING_ENGINE
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    videoResize->videoEnhancer_  = nullptr;
    Status ret = videoResize->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
#endif
}

HWTEST_F(VideoResizeFilterUnitTest, Configure_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    Status ret = videoResize->Configure(parameter);
    videoResize->eventReceiver_ = nullptr;
    ret = videoResize->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

HWTEST_F(VideoResizeFilterUnitTest, GetInputSurface_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    sptr<Surface> surface = videoResize->GetInputSurface();
    videoResize->eventReceiver_ = nullptr;
    surface = videoResize->GetInputSurface();
    EXPECT_EQ(surface, nullptr);
}

HWTEST_F(VideoResizeFilterUnitTest, GetInputSurface_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    sptr<Surface> surface = videoResize->GetInputSurface();
#ifdef USE_VIDEO_PROCESSING_ENGINE
    videoResize->videoEnhancer_ = nullptr;
    surface = videoResize->GetInputSurface();
    EXPECT_EQ(surface, nullptr);
#endif
}

HWTEST_F(VideoResizeFilterUnitTest, SetOutputSurface_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    sptr<Surface> surface = nullptr;
    Status ret = videoResize->SetOutputSurface(surface, 0, 0);
    videoResize->eventReceiver_ = nullptr;
    ret = videoResize->SetOutputSurface(surface, 0, 0);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

HWTEST_F(VideoResizeFilterUnitTest, SetOutputSurface_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    sptr<Surface> surface = nullptr;
    Status ret = videoResize->SetOutputSurface(surface, 0, 0);
#ifdef USE_VIDEO_PROCESSING_ENGINE
    videoResize->videoEnhancer_ = nullptr;
    ret = videoResize->SetOutputSurface(surface, 0, 0);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
#else
    videoResize->eventReceiver_ = nullptr;
    ret = videoResize->SetOutputSurface(surface, 0, 0);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
#endif
}

HWTEST_F(VideoResizeFilterUnitTest, DoStart_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize->releaseBufferTask_ = std::make_shared<Task>("VideoResize");
    Status ret = videoResize->DoStart();
    videoResize->eventReceiver_ = nullptr;
    ret = videoResize->DoStart();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

HWTEST_F(VideoResizeFilterUnitTest, DoStop_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize =
        std::make_shared<Pipeline::VideoResizeFilter>("videoResizeFilter", Pipeline::FilterType::VIDEO_CAPTURE);
    videoResize->eventReceiver_ = std::make_shared<MyEventReceiver>();
    videoResize->releaseBufferTask_ = std::make_shared<Task>("VideoResize");
    Status ret = videoResize->DoStop();
    videoResize->eventReceiver_ = nullptr;
    ret = videoResize->DoStop();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
}