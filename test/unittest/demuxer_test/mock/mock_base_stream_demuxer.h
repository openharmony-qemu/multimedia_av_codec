/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef MOCK_BASE_STREAM_DEMUXER_H
#define MOCK_BASE_STREAM_DEMUXER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>

#include "base_stream_demuxer.h"
#include "gmock/gmock.h"
#include "avcodec_common.h"

namespace OHOS {
namespace Media {

class MockBaseStreamDemuxer : public BaseStreamDemuxer {
public:
    MockBaseStreamDemuxer() = default;
    ~MockBaseStreamDemuxer()  = default;

    MOCK_METHOD(Status, Pause, (), (override));
    MOCK_METHOD(Status, Resume, (), (override));
    MOCK_METHOD(Status, Start, (), (override));
    MOCK_METHOD(Status, Stop, (), (override));
    MOCK_METHOD(Status, Flush, (), (override));
    MOCK_METHOD(Status, ResetAllCache, (), (override));
    MOCK_METHOD(Status, CallbackReadAt, (int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
        size_t expectedLen), (override));
    MOCK_METHOD(Status, Init, (const std::string& uri), (override));
    MOCK_METHOD(Status, ResetCache, (int32_t streamId), (override));
    MOCK_METHOD(void, SetDemuxerState, (int32_t streamId, DemuxerState state), (override));
    MOCK_METHOD(int32_t, GetNewAudioStreamID, (), (override));
    MOCK_METHOD(int32_t, GetNewSubtitleStreamID, (), (override));
    MOCK_METHOD(int32_t, GetNewVideoStreamID, (), (override));
    MOCK_METHOD(std::string, SnifferMediaType, (const StreamInfo& streamInfo), (override));
};
} // namespace Media
} // namespace OHOS
#endif // MOCK_BASE_STREAM_DEMUXER_H
