/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SIDE_OUTPUT_SURFACE_PROCESSOR_UNIT_TEST_H
#define SIDE_OUTPUT_SURFACE_PROCESSOR_UNIT_TEST_H

#include "gtest/gtest.h"
#include "side_output_surface_processor.h"

namespace OHOS {
namespace Media {
namespace Pipeline {

class MockPostProcessorCallback : public PostProcessorCallback {
public:
    MockPostProcessorCallback() = default;
    ~MockPostProcessorCallback() override = default;

    void OnError(int32_t errorCode) override
    {
        errorCount++;
        lastErrorCode = errorCode;
    }
    void OnOutputFormatChanged(const Format &format) override
    {
        formatChangedCount++;
    }
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        inputBufferCount++;
        lastInputIndex = index;
    }
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer,
        bool isInSeekContinous = false) override
    {
        outputBufferCount++;
        lastOutputIndex = index;
    }

    void Reset()
    {
        errorCount = 0;
        formatChangedCount = 0;
        inputBufferCount = 0;
        outputBufferCount = 0;
        lastErrorCode = 0;
        lastInputIndex = 0;
        lastOutputIndex = 0;
    }

    int32_t errorCount = 0;
    int32_t formatChangedCount = 0;
    int32_t inputBufferCount = 0;
    int32_t outputBufferCount = 0;
    int32_t lastErrorCode = 0;
    uint32_t lastInputIndex = 0;
    uint32_t lastOutputIndex = 0;
};

class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    void OnEvent(const Event &event) override
    {
        eventCount++;
    }

    int32_t eventCount = 0;
};

class SideOutputSurfaceProcessorUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    std::shared_ptr<SideOutputSurfaceProcessor> processor_ { nullptr };
    std::shared_ptr<MockPostProcessorCallback> callback_ {nullptr};
    std::shared_ptr<MockEventReceiver> receiver_ {nullptr};
};

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // SIDE_OUTPUT_SURFACE_PROCESSOR_UNIT_TEST_H