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

#ifndef HISTREAMER_VIDEO_RESIZE_FILTER_UNIT_TEST_H
#define HISTREAMER_VIDEO_RESIZE_FILTER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "video_resize_filter.h"
#include <cstring>
#include "filter/filter.h"
#include "osal/task/task.h"
#include "common/status.h"
#include "common/log.h"
#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "detail_enhancer_video.h"
#include "detail_enhancer_video_common.h"
#endif

namespace OHOS {
namespace Media {
#ifdef USE_VIDEO_PROCESSING_ENGINE
namespace VideoProcessingEngine {
    class DetailEnhancerVideo;
}
#endif
namespace Pipeline {
class VideoResizeFilterUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<Pipeline::VideoResizeFilter> videoResize_{ nullptr };
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        std::cout << "event receiver constructor" << std::endl;
    }

    void OnEvent(const Event &event)
    {
        std::cout << event.srcFilter << std::endl;
    }
};

class TestFilterCallback : public Pipeline::FilterCallback {
public:
    explicit TestFilterCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter,
        Pipeline::FilterCallBackCommand cmd, Pipeline::StreamType outType)
    {
        return Status::OK;
    }
};

class MyEventReceiver : public Pipeline::EventReceiver {
public:
    ~MyEventReceiver() = default;
    void OnEvent(const Event& event)
    {
        return;
    }
};

class TestFilter : public Filter {
public:
    TestFilter():Filter("TestFilter", FilterType::FILTERTYPE_SOURCE) {}
    ~TestFilter() = default;
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
                            const std::shared_ptr<FilterLinkCallback>& callback)
    {
        (void)inType;
        (void)meta;
        (void)callback;
        return Status::ERROR_NULL_POINTER;
    }
protected:
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_VIDEO_RESIZE_FILTER_UNIT_TEST_H