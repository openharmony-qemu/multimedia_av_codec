
// /*
//  * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  *     http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */

// #ifndef MOCK_DATA_SOURCE_H
// #define MOCK_DATA_SOURCE_H

// #include <gmock/gmock.h>
// #include <gtest/gtest.h>
// #include "common/status.h"
// #include "plugin/plugin_buffer.h"
// #include "plugin/plugin_definition.h"
// #include "demuxer/demuxer_plugin_manager.h"

// namespace OHOS {
// namespace Media {
// class MockDataSource : public DataSourceImpl {
// public:
//     explicit MockDataSource(const std::shared_ptr<BaseStreamDemuxer>& stream, int32_t streamID);
//     ~MockDataSource();

//     MOCK_METHOD(Status, ReadAt, (int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen));
//     // MOCK_METHOD(Status, ReadAt, (int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen), (override));
//     // MOCK_METHOD(Status, GetSize, (uint64_t& size), (override));
//     // MOCK_METHOD(Seekable, GetSeekable, (), (override));
//     // MOCK_METHOD(int32_t, GetStreamID, (), (override));
//     // MOCK_METHOD(bool, IsDash, (), (override));
// };
// } // namespace Media
// } // namespace OHOS
// #endif // MOCK_DATA_SOURCE_H