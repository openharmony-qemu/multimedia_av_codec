/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef AUDIO_DECODER_FILTER_UNITTEST_H
#define AUDIO_DECODER_FILTER_UNITTEST_H
#include "gtest/gtest.h"
#include "mock/mock_imediakeysessionServices.h"
#include "mock/audio_decoder_adapter.h"
#include "audio_decoder_filter.h"

namespace OHOS {
namespace Media {
constexpr const int64_t SAMEPLE_S32LE = 1;
using namespace OHOS::Media::Pipeline;
class AudioDecoderFilterUnitTest : public testing::Test {
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
    std::shared_ptr<AudioDecoderFilter> audioDecoderFilter_ { nullptr };
};
class MockEventReceiver : public EventReceiver {
public:
    ~MockEventReceiver() override = default;
    MOCK_METHOD(void, OnEvent, (const Event& event), (override));
    MOCK_METHOD(void, OnDfxEvent, (const DfxEvent& event), (override));
    MOCK_METHOD(void, NotifyRelease, (), (override));
    MOCK_METHOD(void, OnMemoryUsageEvent, (const DfxEvent& event), (override));
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_DECODER_FILTER_UNITTEST_H