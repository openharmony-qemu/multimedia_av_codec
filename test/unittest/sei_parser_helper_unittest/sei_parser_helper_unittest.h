/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef AVSEI_PARSER_HELPER_UNITTEST_H
#define AVSEI_PARSER_HELPER_UNITTEST_H

#include "gtest/gtest.h"
#include "mock/avbuffer.h"
#include "mock/media_sync_manager.h"
#include "sei_parser_helper.h"
#include "mock/mock_plugin_buffer_memory.h"
namespace OHOS {
namespace Media {
namespace Pipeline {

class AvcSeiParserHelperUnittest : public testing::Test {
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
    std::shared_ptr<AvcSeiParserHelper> avcSeiParserHelper_{ nullptr };
};

class SeiParserHelperUnittest : public testing::Test {
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
    std::shared_ptr<SeiParserHelper> SeiParserHelper_{ nullptr };
};

class SeiParserListenerUnittest : public testing::Test {
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
    std::shared_ptr<SeiParserListener> SeiParserListener_{ nullptr };
};

class HevcSeiParserHelperUnittest : public testing::Test {
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
    std::shared_ptr<HevcSeiParserHelper> HevcSeiParserHelper_{nullptr};
};

class MockAVBufferQueueProducer : public AVBufferQueueProducer {
public:
    MockAVBufferQueueProducer() = default;
    ~MockAVBufferQueueProducer() override {};
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status, RequestBuffer,
        (std::shared_ptr<AVBuffer>& outBuffer, const AVBufferConfig& config, int32_t timeoutMs),
        (override));
    MOCK_METHOD(Status, PushBuffer,
        (const std::shared_ptr<AVBuffer>& inBuffer, bool available),
        (override));
    MOCK_METHOD(Status, ReturnBuffer,
        (const std::shared_ptr<AVBuffer>& inBuffer, bool available),
        (override));
    MOCK_METHOD(Status, AttachBuffer,
        (std::shared_ptr<AVBuffer>& inBuffer, bool isFilled),
        (override));
    MOCK_METHOD(Status, DetachBuffer,
        (const std::shared_ptr<AVBuffer>& outBuffer),
        (override));
    MOCK_METHOD(Status, SetBufferFilledListener,
        (sptr<IBrokerListener>& listener),
        (override));
    MOCK_METHOD(Status, RemoveBufferFilledListener,
        (sptr<IBrokerListener>& listener),
        (override));
    MOCK_METHOD(Status, SetBufferAvailableListener,
        (sptr<IProducerListener>& listener),
        (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf,
        (std::function<bool(const std::shared_ptr<AVBuffer>&)> pred),
        (override));
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // AVSEI_PARSER_HELPER_UNITTEST_H
