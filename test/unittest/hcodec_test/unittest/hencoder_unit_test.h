/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HENCODER_UNIT_TEST_H
#define HENCODER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "hcodec.h"

namespace OHOS {
namespace MediaAVCodec {
class HEncoderCallback : public MediaCodecCallback {
public:
    HEncoderCallback() {}
    ~HEncoderCallback() {}
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
};

class HEncoderPreparingUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    sptr<Surface> CreateProducerSurface();
    sptr<Surface> CreateConsumerSurface();
};

class HEncoderUserCallingUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    bool ConfigureAvcEncoder(std::shared_ptr<HCodec>& encoder);
    bool ConfigureHevcEncoder(std::shared_ptr<HCodec>& encoder);
    bool SetCallbackToEncoder(std::shared_ptr<HCodec>& encoder);
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif