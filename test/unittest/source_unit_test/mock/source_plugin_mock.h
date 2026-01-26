/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SOURCE_PLUGIN_MOCK_H
#define SOURCE_PLUGIN_MOCK_H

#include <gmock/gmock.h>

#include "plugin/plugin_base.h"
#include "plugin/source_plugin.h"
#include "common/media_source.h"
#include "plugin/plugin_event.h"
#include "meta/media_types.h"

namespace OHOS {
namespace Media {
class MockSourcePlugin : public Plugins::SourcePlugin {
public:
    MockSourcePlugin(std::string name) : SourcePlugin(name){};
    ~MockSourcePlugin() = default;
//SourcePlugin
    MOCK_METHOD(Status, SetSource, (std::shared_ptr<Plugins::MediaSource> source), ());
    MOCK_METHOD(Status, Read, (int32_t streamID, std::shared_ptr<Plugins::Buffer> &buffer,
        uint64_t offset, size_t expectedLen), ());
    MOCK_METHOD(Status, Read, (std::shared_ptr<Plugins::Buffer> &buffer, uint64_t offset,
        size_t expectedLen), ());
    MOCK_METHOD(Status, GetSize, (uint64_t & size), ());
    MOCK_METHOD(Plugins::Seekable, GetSeekable, (), ());
    MOCK_METHOD(Status, SeekTo, (uint64_t offset), ());
    MOCK_METHOD(Status, Reset, (), ());
    MOCK_METHOD(void, SetDemuxerState, (int32_t streamId), ());
    MOCK_METHOD(void, SetDownloadErrorState, (), ());
    MOCK_METHOD(void, SetBundleName, (const std::string &bundleName), ());
    MOCK_METHOD(Status, GetDownloadInfo, (Plugins::DownloadInfo & downloadInfo), ());
    MOCK_METHOD(Status, GetPlaybackInfo, (Plugins::PlaybackInfo & playbackInfo), ());
    MOCK_METHOD(Status, GetBitRates, (std::vector<uint32_t> & bitRates), ());
    MOCK_METHOD(Status, SetStartPts, (int64_t startPts), ());
    MOCK_METHOD(Status, SetExtraCache, (uint64_t cacheDuration), ());
    MOCK_METHOD(Status, SelectBitRate, (uint32_t bitRate), ());
    MOCK_METHOD(Status, AutoSelectBitRate, (uint32_t bitRate), ());
    MOCK_METHOD(bool, IsSeekToTimeSupported, (), ());
    MOCK_METHOD(Status, SeekToTime, (int64_t seekTime, Plugins::SeekMode mode), ());
    MOCK_METHOD(Status, GetDuration, (int64_t& duration), ());
    MOCK_METHOD(bool, IsNeedPreDownload, (), ());
    MOCK_METHOD(Status, SetReadBlockingFlag, (bool isReadBlockingAllowed), ());
    MOCK_METHOD(void, SetInterruptState, (bool isInterruptNeeded), ());
    MOCK_METHOD(Status, SetCurrentBitRate, (int32_t bitRate, int32_t streamID), ());
    MOCK_METHOD(Status, GetStreamInfo, (std::vector<Plugins::StreamInfo> & streams), ());
    MOCK_METHOD(Status, SelectStream, (int32_t streamID), ());
    MOCK_METHOD(Status, Pause, (), ());
    MOCK_METHOD(Status, Resume, (), ());
    MOCK_METHOD(void, SetEnableOnlineFdCache, (bool isEnableFdCache), ());
    MOCK_METHOD(size_t, GetSegmentOffset, (), ());
    MOCK_METHOD(bool, GetHLSDiscontinuity, (), ());
    MOCK_METHOD(Status, StopBufferring, (bool isAppBackground), ());
    MOCK_METHOD(void, WaitForBufferingEnd, (), ());
    MOCK_METHOD(bool, SetSourceInitialBufferSize, (int32_t offset, int32_t size), ());
    MOCK_METHOD(void, NotifyInitSuccess, (), ());
    MOCK_METHOD(bool, IsLocalFd, (), ());
    MOCK_METHOD(uint64_t, GetCachedDuration, (), ());
    MOCK_METHOD(void, RestartAndClearBuffer, (), ());
    MOCK_METHOD(bool, IsFlvLive, (), ());
    MOCK_METHOD(bool, IsHlsFmp4, (), ());
    MOCK_METHOD(uint64_t, GetMemorySize, (), ());
    MOCK_METHOD(std::string, GetContentType, (), ());
//PluginBase
    MOCK_METHOD(void, OnEvent, (const Plugins::PluginEvent &event), ());
    MOCK_METHOD(void, OnDfxEvent, (const Plugins::PluginDfxEvent &event), ());
    MOCK_METHOD(void, SetSelectBitRateFlag, (bool flag, uint32_t desBitRate), ());
    MOCK_METHOD(bool, CanAutoSelectBitRate, (), ());

    MOCK_METHOD(Status, Init, (), ());
    MOCK_METHOD(Status, Deinit, (), ());
    MOCK_METHOD(Status, Prepare, (), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, GetParameter, (std::shared_ptr<Meta> &meta), ());
    MOCK_METHOD(Status, SetParameter, (const std::shared_ptr<Meta> &meta), ());
    MOCK_METHOD(Status, SetCallback, (Plugins::Callback* cb), ());
};
}  // namespace Media
}  // namespace OHOS
#endif // SOURCE_PLUGIN_MOCK_H