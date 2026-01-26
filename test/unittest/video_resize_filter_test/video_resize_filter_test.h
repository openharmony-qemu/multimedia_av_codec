/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef VIDEO_RESIZE_FILTER_TEST_H
#define VIDEO_RESIZE_FILTER_TEST_H

#include "gtest/gtest.h"
#include "mock/task_mock.h"
#include "mock/filter_mock.h"
#include "mock/enchancer_video_mock.h"
#include "mock/surface_mock.h"
#include "video_resize_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoResizeFilterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    std::shared_ptr<VideoResizeFilter> videoResizeFilter_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockEventReceiver> eventReceiver_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockFilterCallback> filterCallback_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockFilterLinkCallback> onLinkedResultCallback_{ nullptr };
    std::shared_ptr<OHOS::Media::Pipeline::MockFilter> nextFilter_{ nullptr };
    std::shared_ptr<OHOS::Media::MockTask> releaseBufferTask_{ nullptr };
    sptr<OHOS::MockSurface> mockSurface_{ nullptr };
#ifdef USE_VIDEO_PROCESSING_ENGINE
    std::shared_ptr<OHOS::Media::VideoProcessingEngine::MockDetailEnhancerVideo> videoEnhancer_{ nullptr };
#endif
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // VIDEO_RESIZE_FILTER_TEST_H