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

#include <gmock/gmock.h>

#include "plugin/plugin_base.h"
#include "plugin/source_plugin.h"

namespace OHOS {
namespace Media {
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

    void SetCallbackWrap(Callback *callbackWrap)
    {
        callbackWrap_ = callbackWrap;
    }

private:
    Callback *callbackWrap_{ nullptr };
};

class Source : public Plugins::Callback {
public:
    Source() = default;
    virtual ~Source() = default;

    virtual Status SetSource(const std::shared_ptr<Plugins::MediaSource>& source) = 0;
    virtual void SetBundleName(const std::string& bundleName) = 0;
    virtual Status Prepare() = 0;
    virtual Status Start() = 0;
    virtual Status Stop() = 0;
    virtual Status Pause() = 0;
    virtual Status Resume() = 0;
    virtual Status SetReadBlockingFlag(bool isReadBlockingAllowed) = 0;
    virtual Plugins::Seekable GetSeekable() = 0;
    virtual Status GetSize(uint64_t &fileSize) = 0;
    virtual void OnEvent(const Plugins::PluginEvent &event) = 0;
    virtual void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) = 0;
    virtual bool CanAutoSelectBitRate() = 0;
    virtual bool IsSeekToTimeSupported() = 0;
    virtual int64_t GetDuration() = 0;
    virtual Status SeekToTime(int64_t seekTime, Plugins::SeekMode mode) = 0;
    virtual Status GetBitRates(std::vector<uint32_t>& bitRates) = 0;
    virtual Status SelectBitRate(uint32_t bitRate) = 0;
    virtual Status AutoSelectBitRate(uint32_t bitRate) = 0;
    virtual Status SetStartPts(int64_t startPts) = 0;
    virtual Status SetExtraCache(uint64_t cacheDuration) = 0;
    virtual Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) = 0;
    virtual void SetCallback(Callback* callback) = 0;
    virtual bool IsNeedPreDownload() = 0;
    virtual void SetDemuxerState(int32_t streamId) = 0;
    virtual Status GetStreamInfo(std::vector<Plugins::StreamInfo>& streams) = 0;
    virtual Status Read(int32_t streamID, std::shared_ptr<Plugins::Buffer>& buffer,
        uint64_t offset, size_t expectedLen) = 0;
    virtual Status SeekTo(uint64_t offset) = 0;
    virtual void SetInterruptState(bool isInterruptNeeded) = 0;
    virtual Status GetDownloadInfo(Plugins::DownloadInfo& downloadInfo) = 0;
    virtual Status GetPlaybackInfo(Plugins::PlaybackInfo& playbackInfo) = 0;
    virtual Status SelectStream(int32_t streamID) = 0;
    virtual void SetEnableOnlineFdCache(bool isEnableFdCache) = 0;
    virtual size_t GetSegmentOffset() = 0;
    virtual bool GetHLSDiscontinuity() = 0;
    virtual void WaitForBufferingEnd() = 0;
    virtual bool SetInitialBufferSize(int32_t offset, int32_t size) = 0;
    virtual Status SetPerfRecEnabled(bool perfRecEnabled) = 0;
    virtual void NotifyInitSuccess() = 0;
    virtual bool IsLocalFd() = 0;
    virtual bool IsFlvLiveStream() const = 0;
    virtual uint64_t GetCachedDuration() = 0;
    virtual void RestartAndClearBuffer() = 0;
    virtual bool IsFlvLive() = 0;
    virtual uint64_t GetMemorySize() = 0;
    virtual std::string GetContentType() = 0;
    virtual bool IsHlsFmp4() = 0;
    virtual Status StopBufferring(bool isAppBackground) = 0;

    virtual Status InitPlugin(const std::shared_ptr<Plugins::MediaSource>& source) = 0;
    virtual std::string GetUriSuffix(const std::string& uri) = 0;
    virtual bool GetProtocolByUri() = 0;
    virtual bool ParseProtocol(const std::shared_ptr<Plugins::MediaSource>& source) = 0;
    virtual Status FindPlugin(const std::shared_ptr<Plugins::MediaSource>& source) = 0;
    virtual void ClearData() = 0;
    virtual Status ReadWithPerfRecord(int32_t streamID, std::shared_ptr<Plugins::Buffer>& buffer,
        uint64_t offset, size_t expectedLen) = 0;
    std::string protocol_;
    bool seekToTimeFlag_{false};
    std::string uri_;
    Plugins::Seekable seekable_;

    std::shared_ptr<Plugins::SourcePlugin> plugin_;

    bool isPluginReady_ {false};
    bool isAboveWaterline_ {false};

    std::shared_ptr<CallbackImpl> mediaDemuxerCallback_;
    std::atomic<bool> isInterruptNeeded_{false};
    std::mutex mutex_;
    std::condition_variable seekCond_;
    bool isEnableFdCache_{ true };
    bool perfRecEnabled_ { false };
    bool isFlvLiveStream_ {false};
};
}  // namespace Media
}  // namespace OHOS
#endif
