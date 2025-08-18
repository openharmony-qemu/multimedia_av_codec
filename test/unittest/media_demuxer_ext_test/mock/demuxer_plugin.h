/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_DEMUXER_PLUGIN_H
#define AVCODEC_DEMUXER_PLUGIN_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace OHOS {
namespace Media {
namespace Plugins {
class DemuxerPlugin {
public:
    explicit DemuxerPlugin(std::string name) : name_(std::move(name)) {}
    virtual ~DemuxerPlugin() = default;

    MOCK_METHOD1(SetDataSource, Status(const std::shared_ptr<DataSource>&));
    MOCK_METHOD1(GetMediaInfo, Status(MediaInfo&));
    MOCK_METHOD1(GetUserMeta, Status(std::shared_ptr<Meta>));
    MOCK_METHOD1(SelectTrack, Status(uint32_t));
    MOCK_METHOD1(UnselectTrack, Status(uint32_t));
    MOCK_METHOD2(ReadSample, Status(uint32_t, std::shared_ptr<AVBuffer>));
    MOCK_METHOD3(ReadSample, Status(uint32_t, std::shared_ptr<AVBuffer>, uint32_t));
    MOCK_METHOD2(GetNextSampleSize, Status(uint32_t, int32_t&));
    MOCK_METHOD3(GetNextSampleSize, Status(uint32_t, int32_t&, uint32_t));
    MOCK_METHOD4(SeekTo, Status(int32_t, int64_t, SeekMode, int64_t&));
    MOCK_METHOD2(GetLastPTSByTrackId, Status(uint32_t, int64_t&));

    MOCK_METHOD0(Reset, Status());
    MOCK_METHOD0(Start, Status());
    MOCK_METHOD0(Stop, Status());
    MOCK_METHOD0(Flush, Status());
    MOCK_METHOD0(ResetEosStatus, void());
    MOCK_METHOD0(Pause, void());

    MOCK_METHOD0(IsRefParserSupported, bool());
    MOCK_METHOD2(ParserRefUpdatePos, Status(int64_t, bool));
    MOCK_METHOD0(ParserRefInfo, Status());
    MOCK_METHOD2(GetFrameLayerInfo, Status(std::shared_ptr<AVBuffer>, FrameLayerInfo&));
    MOCK_METHOD2(GetFrameLayerInfo, Status(uint32_t, FrameLayerInfo&));
    MOCK_METHOD2(GetGopLayerInfo, Status(uint32_t, GopLayerInfo&));
    MOCK_METHOD1(GetIFramePos, Status(std::vector<uint32_t>&));
    MOCK_METHOD2(Dts2FrameId, Status(int64_t, uint32_t&));
    MOCK_METHOD2(SeekMs2FrameId, Status(int64_t, uint32_t&));
    MOCK_METHOD2(FrameId2SeekMs, Status(uint32_t, int64_t&));

    MOCK_METHOD1(GetDrmInfo, Status(std::multimap<std::string, std::vector<uint8_t>>&));
    MOCK_METHOD1(SetInterruptState, void(bool));

    MOCK_METHOD3(GetIndexByRelativePresentationTimeUs, Status(uint32_t, uint64_t, uint32_t&));
    MOCK_METHOD3(GetRelativePresentationTimeUsByIndex, Status(uint32_t, uint32_t, uint64_t&));
    MOCK_METHOD1(SetCacheLimit, void(uint32_t));
    
    MOCK_METHOD2(GetCurrentCacheSize, Status(uint32_t, uint32_t&));
    MOCK_METHOD2(GetProbeSize, bool(int32_t&, int32_t&));
    MOCK_METHOD2(SetAsyncReadThreadPriority, Status(const uint32_t, const std::string&));
private:
    std::string name_ {};
};
}
}
}
#endif
