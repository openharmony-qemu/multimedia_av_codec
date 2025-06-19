/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef STREAM_DEMUXER_UNITTEST_H
#define STREAM_DEMUXER_UNITTEST_H

#include "mock/mock_plugin_buffer_memory.h"
#include "mock/source.h"
#include "gtest/gtest.h"
#include "stream_demuxer.h"

namespace OHOS {
namespace Media {
class StreamDemuxerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<StreamDemuxer> streamDemuxer_ = nullptr;
};
class MockSource : public Source {
public:
    MockSource() = default;
    ~MockSource() override = default;

    MOCK_METHOD(void, OnEvent, (const Plugins::PluginEvent &event), (override));
    MOCK_METHOD(void, OnDfxEvent, (const Plugins::PluginDfxEvent &event), (override));
    MOCK_METHOD(void, SetSelectBitRateFlag, (bool flag, uint32_t desBitRate), (override));
    MOCK_METHOD(bool, CanAutoSelectBitRate, (), (override));

    MOCK_METHOD(Status, SetSource, (const std::shared_ptr<MediaSource> &source), (override));
    MOCK_METHOD(void, SetBundleName, (const std::string &bundleName), (override));
    MOCK_METHOD(Status, Prepare, (), (override));
    MOCK_METHOD(Status, Start, (), (override));
    MOCK_METHOD(Status, Stop, (), (override));
    MOCK_METHOD(Status, Pause, (), (override));
    MOCK_METHOD(Status, Resume, (), (override));
    MOCK_METHOD(Status, SetReadBlockingFlag, (bool isReadBlockingAllowed), (override));
    MOCK_METHOD(Plugins::Seekable, GetSeekable, (), (override));
    MOCK_METHOD(Status, GetSize, (uint64_t & fileSize), (override));
    MOCK_METHOD(bool, IsSeekToTimeSupported, (), (override));
    MOCK_METHOD(bool, IsLocalFd, (), (override));
    MOCK_METHOD(int64_t, GetDuration, (), (override));
    MOCK_METHOD(Status, SeekToTime, (int64_t seekTime, SeekMode mode), (override));
    MOCK_METHOD(Status, SeekTo, (uint64_t offset), (override));
    MOCK_METHOD(Status, GetBitRates, (std::vector<uint32_t> & bitRates), (override));
    MOCK_METHOD(Status, SelectBitRate, (uint32_t bitRate), (override));
    MOCK_METHOD(Status, AutoSelectBitRate, (uint32_t bitRate), (override));
    MOCK_METHOD(Status, StopBufferring, (bool flag), (override));
    MOCK_METHOD(Status, SetStartPts, (int64_t startPts), (override));
    MOCK_METHOD(Status, SetExtraCache, (uint64_t cacheDuration), (override));
    MOCK_METHOD(Status, SetCurrentBitRate, (int32_t bitRate, int32_t streamID), (override));
    MOCK_METHOD(void, SetCallback, (Callback * callback), (override));
    MOCK_METHOD(bool, IsNeedPreDownload, (), (override));
    MOCK_METHOD(void, SetDemuxerState, (int32_t streamId), (override));
    MOCK_METHOD(Status, GetStreamInfo, (std::vector<StreamInfo> & streams), (override));
    MOCK_METHOD(Status, Read, (int32_t streamID, std::shared_ptr<Buffer> &buffer,
        uint64_t offset, size_t expectedLen), (override));
    MOCK_METHOD(void, SetInterruptState, (bool isInterruptNeeded), (override));
    MOCK_METHOD(Status, GetDownloadInfo, (DownloadInfo & downloadInfo), (override));
    MOCK_METHOD(Status, GetPlaybackInfo, (PlaybackInfo & playbackInfo), (override));
    MOCK_METHOD(Status, SelectStream, (int32_t streamID), (override));
    MOCK_METHOD(void, SetEnableOnlineFdCache, (bool isEnableFdCache), (override));
    MOCK_METHOD(size_t, GetSegmentOffset, (), (override));
    MOCK_METHOD(bool, GetHLSDiscontinuity, (), (override));
    MOCK_METHOD(void, WaitForBufferingEnd, (), (override));
    MOCK_METHOD(bool, SetInitialBufferSize, (int32_t offset, int32_t size), (override));
    MOCK_METHOD(Status, SetPerfRecEnabled, (bool perfRecEnabled), (override));
    MOCK_METHOD(void, NotifyInitSuccess, (), (override));
    MOCK_METHOD(uint64_t, GetCachedDuration, (), (override));
    MOCK_METHOD(void, RestartAndClearBuffer, (), (override));
    MOCK_METHOD(bool, IsFlvLive, (), (override));
    MOCK_METHOD(bool, IsFlvLiveStream, (), (override, const));
    MOCK_METHOD(uint64_t, GetMemorySize, (), (override));
    MOCK_METHOD(std::string, GetContentType, (), (override));
    MOCK_METHOD(bool, IsHlsFmp4, (), (override));
    MOCK_METHOD(Status, InitPlugin, (const std::shared_ptr<Plugins::MediaSource>& source), (override));
    MOCK_METHOD(std::string, GetUriSuffix, (const std::string& uri), (override));
    MOCK_METHOD(bool, GetProtocolByUri, (), (override));
    MOCK_METHOD(bool, ParseProtocol, (const std::shared_ptr<Plugins::MediaSource>& source), (override));
    MOCK_METHOD(Status, FindPlugin, (const std::shared_ptr<Plugins::MediaSource>& source), (override));
    MOCK_METHOD(void, ClearData, (), (override));
    MOCK_METHOD(Status, ReadWithPerfRecord, (int32_t streamID, std::shared_ptr<Plugins::Buffer>& buffer,
        uint64_t offset, size_t expectedLen), (override));
};
}
}

#endif