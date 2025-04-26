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
#ifndef SUPER_RESOLUTION_POST_PROCESSOR_UNIT_TEST_H
#define SUPER_RESOLUTION_POST_PROCESSOR_UNIT_TEST_H

#include "gtest/gtest.h"
#include "mock/algorithm_video.h"
#include "super_resolution_post_processor.cpp"

namespace OHOS {
namespace Media {
namespace Pipeline {

class MockPostProcessorCallback : public PostProcessorCallback {
public:
    void OnError(int32_t errorCode) override {}
    void OnOutputFormatChanged(const Format &format) override {}
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {}
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        buffer_ = buffer;
    }

    std::shared_ptr<AVBuffer> buffer_ {nullptr};
};

class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    void OnEvent(const Event &event){};
};

class SuperResolutionPostProcessorUnitTest : public testing::Test {
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
    std::shared_ptr<SuperResolutionPostProcessor> postProcessor_ { nullptr };
    std::shared_ptr<MockPostProcessorCallback> callback_ {nullptr};
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // SUPER_RESOLUTION_POST_PROCESSOR_UNIT_TEST_H
