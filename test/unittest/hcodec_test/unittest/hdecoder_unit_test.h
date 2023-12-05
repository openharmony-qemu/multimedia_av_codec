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

#ifndef HDECODER_UNIT_TEST_H
#define HDECODER_UNIT_TEST_H

#include <mutex>
#include <condition_variable>
#include <list>
#include "gtest/gtest.h"
#include "hcodec.h"

namespace OHOS {
namespace MediaAVCodec {
class HDecoderSignal {
public:
    std::mutex inputMtx_;
    std::condition_variable inputCond_;
    std::list<std::pair<uint32_t, std::shared_ptr<AVBuffer>>> inputList_;
};

class HDecoderCallback : public MediaCodecCallback {
public:
    explicit HDecoderCallback(std::shared_ptr<HDecoderSignal>& obj):signal_(obj) {}
    ~HDecoderCallback() override = default;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

    std::shared_ptr<HDecoderSignal> signal_;
};

class HDecoderPreparingUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    sptr<Surface> CreateOutputSurface();
};

class HDecoderUserCallingUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    bool ConfigureDecoder(std::shared_ptr<HCodec>& decoder);
    bool SetOutputSurfaceToDecoder(std::shared_ptr<HCodec>& decoder);
    bool SetCallbackToDecoder(std::shared_ptr<HCodec>& decoder);

    class Listener : public IBufferConsumerListener {
    public:
        explicit Listener(HDecoderUserCallingUnitTest *test) : mTest(test) {}
        void OnBufferAvailable() override;
    private:
        HDecoderUserCallingUnitTest *mTest;
    };

    sptr<Surface> CreateOutputSurface();
    sptr<Surface> mConsumer;

    std::shared_ptr<HDecoderSignal> signal_;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif