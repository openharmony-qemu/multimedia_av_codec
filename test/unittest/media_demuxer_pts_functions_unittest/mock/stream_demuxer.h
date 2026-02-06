/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef BASE_STREAM_DEMUXER_H
#define BASE_STREAM_DEMUXER_H
#ifndef STREAM_DEMUXER_H
#define STREAM_DEMUXER_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>


namespace OHOS {
namespace Media {
enum class DemuxerState {
    DEMUXER_STATE_NULL,
    DEMUXER_STATE_PARSE_HEADER,
    DEMUXER_STATE_PARSE_FIRST_FRAME,
    DEMUXER_STATE_PARSE_FRAME
};

class BaseStreamDemuxer {
public:
    BaseStreamDemuxer() = default;
    ~BaseStreamDemuxer() = default;

    MOCK_METHOD1(Init, Status(const std::string& uri));
    MOCK_METHOD0(Pause, Status());
    MOCK_METHOD0(Resume, Status());
    MOCK_METHOD0(Start, Status());
    MOCK_METHOD0(Stop, Status());
    MOCK_METHOD0(Flush, Status());
    MOCK_METHOD1(ResetCache, Status(int32_t streamID));
    MOCK_METHOD0(ResetAllCache, Status());

    MOCK_METHOD1(SetSource, void(const std::shared_ptr<Source>& source));

    MOCK_METHOD4(
        CallbackReadAt, Status(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen));

    MOCK_METHOD2(SetDemuxerState, void(int32_t streamId, DemuxerState state));
    MOCK_METHOD1(SetBundleName, void(const std::string& bundleName));
    MOCK_METHOD1(SetIsIgnoreParse, void(bool state));
    MOCK_METHOD0(GetIsIgnoreParse, bool());
    MOCK_METHOD0(GetSeekable, Plugins::Seekable());
    MOCK_METHOD1(SetInterruptState, void(bool isInterruptNeeded));
    MOCK_METHOD1(SnifferMediaType, std::string(int32_t streamID));

    MOCK_CONST_METHOD0(IsDash, bool());
    MOCK_METHOD1(SetIsDash, void(bool flag));

    MOCK_METHOD1(SetNewAudioStreamID, Status(int32_t streamID));
    MOCK_METHOD1(SetNewVideoStreamID, Status(int32_t streamID));
    MOCK_METHOD1(SetNewSubtitleStreamID, Status(int32_t streamID));
    MOCK_METHOD0(GetNewVideoStreamID, int32_t());
    MOCK_METHOD0(GetNewAudioStreamID, int32_t());
    MOCK_METHOD0(GetNewSubtitleStreamID, int32_t());
    MOCK_METHOD0(CanDoChangeStream, bool());
    MOCK_METHOD1(SetChangeFlag, void(bool flag));
    MOCK_METHOD0(GetIsExtSubtitle, bool());
    MOCK_METHOD1(SetIsExtSubtitle, void(bool flag));
    MOCK_METHOD2(SetSourceInitialBufferSize, bool(int32_t offset, int32_t size));
    MOCK_METHOD1(SetSourceType, void(SourceType type));
    MOCK_METHOD0(GetIsDataSrcNoSeek, bool());
};

class StreamDemuxer : public BaseStreamDemuxer {
public:
    StreamDemuxer() = default;
    ~StreamDemuxer() = default;
};
}
}
#endif
#endif
