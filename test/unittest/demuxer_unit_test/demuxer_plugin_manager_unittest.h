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

#ifndef DEMUXER_PLUGIN_MANAGER_UNITTEST_H
#define DEMUXER_PLUGIN_MANAGER_UNITTEST_H
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>
#include "base_stream_demuxer.h"
#include "avcodec_common.h"
#include "demuxer_plugin_manager.h"

namespace OHOS {
namespace Media {
class MockBaseStreamDemuxer;
class DemuxerPluginManagerUnittest : public testing::Test {
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
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager_{ nullptr };
    std::shared_ptr<DataSourceImpl> dataSourceImpl_{ nullptr };
    std::shared_ptr<MockBaseStreamDemuxer> streamDemuxer_{ nullptr };
};
class MockBaseStreamDemuxer : public BaseStreamDemuxer {
public:
    MockBaseStreamDemuxer() = default;
    ~MockBaseStreamDemuxer()  = default;

    MOCK_METHOD(Status, Pause, (), ());
    MOCK_METHOD(Status, Resume, (), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, Flush, (), ());
    MOCK_METHOD(Status, ResetAllCache, (), ());
    MOCK_METHOD(Status, CallbackReadAt, (int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
        size_t expectedLen), ());
    MOCK_METHOD(Status, Init, (const std::string& uri), ());
    MOCK_METHOD(Status, ResetCache, (int32_t streamId), ());
    MOCK_METHOD(void, SetDemuxerState, (int32_t streamId, DemuxerState state), ());
    MOCK_METHOD(int32_t, GetNewAudioStreamID, (), ());
    MOCK_METHOD(int32_t, GetNewSubtitleStreamID, (), ());
    MOCK_METHOD(int32_t, GetNewVideoStreamID, (), ());
    MOCK_METHOD(std::string, SnifferMediaType, (const StreamInfo& streamInfo), ());
    MOCK_METHOD(bool, SetSourceInitialBufferSize, (int32_t offset, int32_t size), (override));
};
}  // namespace Media
}  // namespace OHOS
#endif