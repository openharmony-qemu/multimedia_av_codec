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

#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "osal/task/task.h"
#include "osal/utils/util.h"
#include "common/media_source.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_event.h"
#include "plugin/source_plugin.h"
#include "meta/media_types.h"
#include "demuxer/media_demuxer.h"
#include "performance_utils.h"

namespace OHOS {
namespace Media {
using SourceType = OHOS::Media::Plugins::SourceType;
using MediaSource = OHOS::Media::Plugins::MediaSource;

class CallbackImpl : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
    }
    
    void OnDfxEvent(const Plugins::PluginDfxEvent &event) override
    {
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override
    {
    }

    bool CanAutoSelectBitRate() override
    {
        return false;
    }

    void SetCallbackWrap(Callback* callbackWrap)
    {
    }

private:
    Callback* callbackWrap_ {nullptr};
};

class Source : public Plugins::Callback {
public:
    Source() = default;
    ~Source() = default;
    MOCK_METHOD(Status, SetSource, (const std::shared_ptr<MediaSource>& source), ());
    MOCK_METHOD(void, SetBundleName, (const std::string& bundleName), ());
    MOCK_METHOD(Status, Prepare, (), ());
    MOCK_METHOD(Status, Start, (), ());
    MOCK_METHOD(Status, Stop, (), ());
    MOCK_METHOD(Status, Pause, (), ());
    MOCK_METHOD(Status, Resume, (), ());
    MOCK_METHOD(Status, SetReadBlockingFlag, (bool isReadBlockingAllowed), ());
    MOCK_METHOD(Plugins::Seekable, GetSeekable, (), ());
    MOCK_METHOD(Status, GetSize, (uint64_t &fileSize), ());
    MOCK_METHOD(void, OnEvent, (const Plugins::PluginEvent &event), ());
    MOCK_METHOD(void, SetSelectBitRateFlag, (bool flag, uint32_t desBitRate), ());
    MOCK_METHOD(bool, CanAutoSelectBitRate, (), ());
    MOCK_METHOD(bool, IsSeekToTimeSupported, (), ());
    MOCK_METHOD(int64_t, GetDuration, (), ());
    MOCK_METHOD(Status, SeekToTime, (int64_t seekTime, SeekMode mode), ());
    MOCK_METHOD(Status, GetBitRates, (std::vector<uint32_t>& bitRates), ());
    MOCK_METHOD(Status, SelectBitRate, (uint32_t bitRate), ());
    MOCK_METHOD(Status, AutoSelectBitRate, (uint32_t bitRate), ());
    MOCK_METHOD(Status, StopBufferring, (bool flag), ());
    MOCK_METHOD(Status, SetStartPts, (int64_t startPts), ());
    MOCK_METHOD(Status, SetExtraCache, (uint64_t cacheDuration), ());
    MOCK_METHOD(Status, SetCurrentBitRate, (int32_t bitRate, int32_t streamID), ());
    MOCK_METHOD(void, SetCallback, (Callback* callback), ());
    MOCK_METHOD(bool, IsNeedPreDownload, (), ());
    MOCK_METHOD(void, SetDemuxerState, (int32_t streamId), ());
    MOCK_METHOD(Status, GetStreamInfo, (std::vector<StreamInfo>& streams), ());
    MOCK_METHOD(Status, Read, (int32_t streamID, std::shared_ptr<Buffer>& buffer,
        uint64_t offset, size_t expectedLen), ());
    MOCK_METHOD(Status, SeekTo, (uint64_t offset), ());
    MOCK_METHOD(void, SetInterruptState, (bool isInterruptNeeded), ());
    MOCK_METHOD(Status, GetDownloadInfo, (DownloadInfo& downloadInfo), ());
    MOCK_METHOD(Status, GetPlaybackInfo, (PlaybackInfo& playbackInfo), ());
    MOCK_METHOD(Status, SelectStream, (int32_t streamID), ());
    MOCK_METHOD(void, SetEnableOnlineFdCache, (bool isEnableFdCache), ());
    MOCK_METHOD(size_t, GetSegmentOffset, (), ());
    MOCK_METHOD(bool, GetHLSDiscontinuity, (), ());
    MOCK_METHOD(void, WaitForBufferingEnd, (), ());
    MOCK_METHOD(bool, SetInitialBufferSize, (int32_t offset, int32_t size), ());
    MOCK_METHOD(Status, SetPerfRecEnabled, (bool perfRecEnabled), ());
    MOCK_METHOD(void, NotifyInitSuccess, (), ());
    MOCK_METHOD(bool, IsLocalFd, (), ());
    MOCK_METHOD(bool, IsFlvLiveStream, (), (const));
    MOCK_METHOD(uint64_t, GetCachedDuration, (), ());
    MOCK_METHOD(void, RestartAndClearBuffer, (), ());
    MOCK_METHOD(bool, IsFlvLive, (), ());
    MOCK_METHOD(uint64_t, GetMemorySize, (), ());
    MOCK_METHOD(std::string, GetContentType, (), ());
    MOCK_METHOD(bool, IsHlsFmp4, (), ());
    MOCK_METHOD(Status, InitPlugin, (const std::shared_ptr<MediaSource>& source), ());
    static std::string GetUriSuffix(const std::string& uri)
    {
        return "";
    }
    MOCK_METHOD(bool, GetProtocolByUri, (), ());
    MOCK_METHOD(bool, ParseProtocol, (const std::shared_ptr<MediaSource>& source), ());
    MOCK_METHOD(Status, FindPlugin, (const std::shared_ptr<MediaSource>& source), ());
    MOCK_METHOD(void, ClearData, (), ());
    MOCK_METHOD(Status, ReadWithPerfRecord, (int32_t streamID, std::shared_ptr<Buffer>& buffer,
        uint64_t offset, size_t expectedLen), ());

    std::string protocol_;
    bool seekToTimeFlag_{false};
    std::string uri_;
    Plugins::Seekable seekable_;

    std::shared_ptr<Plugins::SourcePlugin> plugin_;

    std::shared_ptr<Plugins::PluginInfo> pluginInfo_{};
    bool isPluginReady_ {false};
    bool isAboveWaterline_ {false};

    std::shared_ptr<CallbackImpl> mediaDemuxerCallback_;
    std::atomic<bool> isInterruptNeeded_{false};
    std::mutex mutex_;
    std::condition_variable seekCond_;
    bool isEnableFdCache_{ true };
    bool perfRecEnabled_ { false };
    PerfRecorder perfRecorder_ {};
    bool isFlvLiveStream_ {false};
};
} // namespace Media
} // namespace OHOS
#endif