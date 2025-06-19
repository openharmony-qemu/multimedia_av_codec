/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "video_capture_filter_unittest.h"
#include "mock/avcodec_trace.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing::ext;
using namespace testing;

void VideoCaptureFilterUnitTest::SetUpTestCase(void) {}

void VideoCaptureFilterUnitTest::TearDownTestCase(void) {}

void VideoCaptureFilterUnitTest::SetUp(void) {}

void VideoCaptureFilterUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test OnAudioSessionChecked
 * @tc.number: OnAudioSessionChecked_001
 * @tc.desc  : Test it1 != playerMap_.end()
 *             Test player != nullptr && player->IsPlaying()
 */
HWTEST_F(VideoCaptureFilterUnitTest, OnLinkedResult_001, TestSize.Level0)
{
    auto videoCaptureFilter = std::make_shared<VideoCaptureFilter>("testVideoCaptureFilter", FilterType::VIDEO_CAPTURE);
    auto linkCallback = std::make_shared<VideoCaptureFilterLinkCallback>(videoCaptureFilter);
    auto bufferListener = std::make_shared<ConsumerSurfaceBufferListener>(videoCaptureFilter);
    sptr<AVBufferQueueProducer> outputBufferQueue = new MockAVBufferQueueProducer();
    std::shared_ptr<Meta> meta = nullptr;
    linkCallback->OnLinkedResult(outputBufferQueue, meta);
    linkCallback->OnUnlinkedResult(meta);
    linkCallback->OnUpdatedResult(meta);
    bufferListener->OnBufferAvailable();
    auto filterPtr = linkCallback->videoCaptureFilter_.lock();
    ASSERT_NE(filterPtr, nullptr);
    auto bufferQueue = filterPtr->outputBufferQueueProducer_;
    EXPECT_EQ(bufferQueue, outputBufferQueue);
    
    linkCallback->videoCaptureFilter_ = std::weak_ptr<VideoCaptureFilter>();
    bufferListener->videoCaptureFilter_ = std::weak_ptr<VideoCaptureFilter>();
    linkCallback->OnLinkedResult(outputBufferQueue, meta);
    linkCallback->OnUnlinkedResult(meta);
    linkCallback->OnUpdatedResult(meta);
    bufferListener->OnBufferAvailable();
    delete outputBufferQueue;
    outputBufferQueue = nullptr;
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS