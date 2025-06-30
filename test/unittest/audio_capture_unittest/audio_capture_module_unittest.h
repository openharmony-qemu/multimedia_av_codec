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

#ifndef AUDIO_CAPTURE_MODULE_UNITTEST_H
#define AUDIO_CAPTURE_MODULE_UNITTEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "mock/mock_audio_capture.h"
#include "audio_capture_module.h"

namespace OHOS {
namespace Media {
class AudioCaptureModuleUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    void InitMeta(std::shared_ptr<Meta> meta);

protected:
    std::unique_ptr<MockAudioCapturer> mockAudioCapturer_ {nullptr};
    std::shared_ptr<AudioCaptureModule::AudioCaptureModule> audioCaptureModule_ {nullptr};
};

class MockAudioCaptureModuleCallback : public AudioCaptureModule::AudioCaptureModuleCallback {
public:
    virtual ~MockAudioCaptureModuleCallback() = default;

    MOCK_METHOD(void, OnInterrupt, (const std::string &interruptInfo), (override));
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_CAPTURE_MODULE_UNITTEST_H