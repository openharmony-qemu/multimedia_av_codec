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

#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H

#include <memory>
#include <string>

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
        if (callbackWrap_) {
            callbackWrap_->OnEvent(event);
        }
    }
    
    void OnDfxEvent(const Plugins::PluginDfxEvent &event) override
    {
        if (callbackWrap_) {
            callbackWrap_->OnDfxEvent(event);
        }
    }

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override
    {
        if (callbackWrap_) {
            callbackWrap_->SetSelectBitRateFlag(flag, desBitRate);
        }
    }

    bool CanAutoSelectBitRate() override
    {
        if (callbackWrap_) {
            return callbackWrap_->CanAutoSelectBitRate();
        }
        return false;
    }

    void SetCallbackWrap(Callback* callbackWrap)
    {
        callbackWrap_ = callbackWrap;
    }

private:
    Callback* callbackWrap_ {nullptr};
};

class Source : public Plugins::Callback {
public:
    explicit Source();
    ~Source() override;

    virtual Status SetSource(const std::shared_ptr<MediaSource>& source);
    void SetBundleName(const std::string& bundleName);
    Status Prepare();
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status SetReadBlockingFlag(bool isReadBlockingAllowed);
    Plugins::Seekable GetSeekable();

    Status GetSize(uint64_t &fileSize);

    void OnEvent(const Plugins::PluginEvent &event) override;
    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override;
    bool CanAutoSelectBitRate() override;

    bool IsSeekToTimeSupported();
    bool IsLocalFd();
    int64_t GetDuration();
    Status SeekToTime(int64_t seekTime, SeekMode mode);
    Status SeekTo(uint64_t offset);
    Status GetBitRates(std::vector<uint32_t>& bitRates);
    Status SelectBitRate(uint32_t bitRate);
    Status AutoSelectBitRate(uint32_t bitRate);
    Status SetStartPts(int64_t startPts);
    Status SetExtraCache(uint64_t cacheDuration);
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID);
    void SetCallback(Callback* callback);
    bool IsNeedPreDownload();
    void SetDemuxerState(int32_t streamId);
    Status GetStreamInfo(std::vector<StreamInfo>& streams);
    Status Read(int32_t streamID, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen);
    void SetInterruptState(bool isInterruptNeeded);
    Status GetDownloadInfo(DownloadInfo& downloadInfo);
    Status GetPlaybackInfo(PlaybackInfo& playbackInfo);
    Status SelectStream(int32_t streamID);
    void SetEnableOnlineFdCache(bool isEnableFdCache);
    size_t GetSegmentOffset();
    bool GetHLSDiscontinuity();
    void WaitForBufferingEnd();
    bool SetInitialBufferSize(int32_t offset, int32_t size);
    Status SetPerfRecEnabled(bool perfRecEnabled);
    void NotifyInitSuccess();
    uint64_t GetCachedDuration();
    void RestartAndClearBuffer();
    bool IsFlvLive();
    bool IsFlvLiveStream() const;
    std::string GetContentType();
    bool IsHlsFmp4();
    uint64_t GetMemorySize();
    Status StopBufferring(bool isAppBackground);
    bool IsHlsEnd(int32_t streamId = -1);
    bool IsHls();
    bool IsCloudFd();

private:
    Status InitPlugin(const std::shared_ptr<MediaSource>& source);
    static std::string GetUriSuffix(const std::string& uri);
    bool GetProtocolByUri();
    bool ParseProtocol(const std::shared_ptr<MediaSource>& source);
    Status FindPlugin(const std::shared_ptr<MediaSource>& source);
    void ClearData();
    Status ReadWithPerfRecord(int32_t streamID, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen);

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