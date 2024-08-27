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
#ifndef HISTREAMER_SURFACE_DECODER_FILTER_UNIT_TEST_H
#define HISTREAMER_SURFACE_DECODER_FILTER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "surface_decoder_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class SurfaceDecoderFilterUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    void SetDataSource();

protected:
    std::shared_ptr<SurfaceDecoderFilter> surfaceDecoderFilter_{ nullptr };
    std::shared_ptr<SurfaceDecoderAdapter> mediaCodec_{ nullptr };
    std::shared_ptr<FilterCallback> filterCallback_{nullptr};
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

class TestFilterLinkCallback : public Pipeline::FilterLinkCallback {
public:
    explicit TestFilterLinkCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnLinkedResult" << std::endl;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUnlinkedResult" << std::endl;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUpdatedResult" << std::endl;
    }
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_SURFACE_DECODER_FILTER_UNIT_TEST_H