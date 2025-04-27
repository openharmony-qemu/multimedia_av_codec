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
#ifndef HISTREAMER_DATA_STREAM_SOURCE_PLUGIN_UNIT_TEST_H
#define HISTREAMER_DATA_STREAM_SOURCE_PLUGIN_UNIT_TEST_H

#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include "data_stream_source_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugin {
namespace DataStreamSource {
class TestCallback : public Plugins::Callback {
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        (void) event;
    }
};

class MockMediaDataSource : public IMediaDataSource {
public:
    MockMediaDataSource() = default;
    ~MockMediaDataSource() override = default;

    MOCK_METHOD(int32_t, ReadAt,
        (const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos), (override));
    MOCK_METHOD(int32_t, GetSize, (int64_t &size), (override));
    // Deprecated methods
    MOCK_METHOD(int32_t, ReadAt,
        (int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem), (override));
    MOCK_METHOD(int32_t, ReadAt,
        (uint32_t length, const std::shared_ptr<AVSharedMemory> &mem), (override));
};

class DataStreamSourceUnitTest : public testing::Test {
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
    std::shared_ptr<DataStreamSourcePlugin> plugin_{nullptr};
    std::shared_ptr<MockMediaDataSource> mockDataSrc_{nullptr};
};
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_DATA_STREAM_SOURCE_PLUGIN_UNIT_TEST_H