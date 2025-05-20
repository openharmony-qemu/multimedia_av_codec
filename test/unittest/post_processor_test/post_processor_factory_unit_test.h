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
#ifndef POST_PROCESSOR_FACTORY_UNIT_TEST_H
#define POST_PROCESSOR_FACTORY_UNIT_TEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "video_post_processor_factory.h"

namespace OHOS {
namespace Media {

class MockPostProcessor : public BaseVideoPostProcessor {
public:
    MOCK_METHOD(Status, Init, (), ());
    MOCK_METHOD(Status, Flush, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Release, (), ());
    MOCK_METHOD(Status, Pause, (), ());
    MOCK_METHOD(Status, NotifyEos, (int64_t eosPts), ());

    MOCK_METHOD(sptr<Surface>, GetInputSurface, (), ());
    MOCK_METHOD(Status, SetOutputSurface, (sptr<Surface> surface), ());

    MOCK_METHOD(Status, ReleaseOutputBuffer, (uint32_t index, bool render), ());
    MOCK_METHOD(Status, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestampNs), ());

    MOCK_METHOD(Status, SetCallback, (const std::shared_ptr<PostProcessorCallback> callback), ());
    MOCK_METHOD(Status, SetEventReceiver, (const std::shared_ptr<Pipeline::EventReceiver> &receiver), ());

    MOCK_METHOD(Status, SetParameter, (const Format &format), ());
    MOCK_METHOD(Status, SetPostProcessorOn, (bool isPostProcessorOn), ());
    MOCK_METHOD(Status, SetVideoWindowSize, (int32_t width, int32_t height), ());

    MOCK_METHOD(Status, StartSeekContinous, (), ());
    MOCK_METHOD(Status, StopSeekContinous, (), ());
    MOCK_METHOD(Status, SetFd, (int32_t fd), ());
    MOCK_METHOD(void, SetSeekTime, (int64_t seekTimeUs, PlayerSeekMode mode), ());
    MOCK_METHOD(void, ResetSeekInfo, (), ());
    MOCK_METHOD(Status, SetSpeed, (float speed), ());
};


class PostProcessorFacotoryUnitTest : public testing::Test {
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
    std::shared_ptr<VideoPostProcessorFactory> factory_ {nullptr};
};
}  // namespace Media
}  // namespace OHOS
#endif  // POST_PROCESSOR_FACTORY_UNIT_TEST_H
