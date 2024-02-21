/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AUDIO_CAPTURE_MODULE_UNIT_TEST_H
#define AUDIO_CAPTURE_MODULE_UNIT_TEST_H

#include "gtest/gtest.h"
#include "audio_capture_module.h"

namespace OHOS {
namespace MediaAVCodec {
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

protected:
    // std::shared_ptr<AudioCaptureModule> audioCaptureModule_ {nullptr};

    int32_t appTokenId_{0};
    int32_t appUid_{0};
    int32_t appPid_{0};
    int64_t appFullTokenId_{0};
    int32_t sampleRate_{48000};
    int32_t channel_{1};
    int64_t bitRate_{48000};
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AUDIO_CAPTURE_MODULE_UNIT_TEST_H