/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#ifndef HISTREAMER_HTTP_SOURCE_PLUGIN_H
#define HISTREAMER_HTTP_SOURCE_PLUGIN_H

#include <memory>
#include <mutex>
#include "sniffer/sniffer_coordinator.h"
#include "sniffer/probe_failure_notifier.h"
#include <shared_mutex>
#include "media_downloader.h"
#include "meta/media_types.h"
#include "plugin/source_plugin.h"
#include "download/media_source_loading_request.h"
#include <string_view>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class __attribute__((visibility("default"))) HttpSourcePlugin : public SourcePlugin {
public:
    explicit HttpSourcePlugin(const std::string &name) noexcept;
    ~HttpSourcePlugin() override;
    Status Init() override;
    Status Deinit() override;
    Status Prepare() override;
    Status Reset() override;
    Status Start() override;
    Status Stop() override;
    Status Pause() override;
    Status Resume() override;
    Status GetParameter(std::shared_ptr<Meta> &meta) override;
    Status SetParameter(const std::shared_ptr<Meta> &meta) override;
    Status SetMediaDuration(int64_t mediaDuration) override;
    Status SetCallback(const std::shared_ptr<Callback>& cb) override;
    Status SetSource(std::shared_ptr<MediaSource> source) override;
    Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Seekable GetSeekable() override;
    Status SeekTo(uint64_t offset) override;
    Status SeekToTime(int64_t seekTime, SeekMode mode) override;
    Status MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId) override;
    Status GetDuration(int64_t& duration) override;
    Status GetStartInfo(std::pair<int64_t, bool>& startInfo) override;
    bool IsSeekToTimeSupported() override;
    Status GetBitRates(std::vector<uint32_t>& bitRates) override;
    Status SelectBitRate(uint32_t bitRate) override;
    Status AutoSelectBitRate(uint32_t bitRate) override;
    Status SetStartPts(int64_t startPts) override;
    Status SetExtraCache(uint64_t cacheDuration) override;
    Status SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    Status GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false) override;
    Status SelectStream(int32_t streamID) override;
    void SetDemuxerState(int32_t streamId) override;
    void SetDownloadErrorState() override;
    void SetInterruptState(bool isInterruptNeeded) override;
    Status GetDownloadInfo(DownloadInfo& downloadInfo) override;
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    Status GetPlaybackInfo(PlaybackInfo& playbackInfo) override;
    std::vector<SeekRange> GetSeekableRanges() const override;
    size_t GetSegmentOffset(StreamType streamType = StreamType::VIDEO) override;
    bool GetHLSDiscontinuity() override;
    bool SetSourceInitialBufferSize(int32_t offset, int32_t size) override;
    Status StopBufferring(bool isAppBackground) override;
    void WaitForBufferingEnd() override;
    void NotifyInitSuccess() override;
    uint64_t GetCachedDuration() override;
    void RestartAndClearBuffer() override;
    bool IsFlvLive() override;
    bool IsHlsFmp4() override;
    uint64_t GetMemorySize() override;
    std::string GetContentType() override;
    bool IsHlsEnd(int32_t streamId = -1) override;
    bool IsHls() override;
    std::string GetMaskUrlByReadIndex(int32_t streamId) override;
    void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) override;

private:
    enum class SourceState {
        IDLE,
        PROBING,
        READY,
        PROBE_FAILED,
        CLOSED
    };
    void BeginProbeRoute();
    void SaveProbeFailure(const ProbeFailure& failure, const std::string& url, bool localManifest);
    void CancelProbeFailure();
    Status FinishProbeFailure(const std::shared_ptr<Buffer>& buffer = nullptr);
    std::shared_ptr<ProbeFailureNotifier> GetProbeFailure() const;
    std::shared_ptr<Callback> GetCallback() const;
    void ConfigureDownloader(const std::shared_ptr<MediaDownloader>& downloader,
        const std::shared_ptr<MediaSource>& source, const SniffResult& result, const MediaStreamList& streams);
    void CloseUri(bool isAsync = false);
    std::shared_ptr<PlayStrategy> PlayStrategyInit(std::shared_ptr<MediaSource> source);
    void SetDownloaderBySource(std::shared_ptr<MediaSource> source);
    void SelectInitialStream(MediaStreamList& streams);
    bool IsLocalManifest() const;
    std::shared_ptr<SniffResult> ProbeSource(int32_t appUid);
    std::shared_ptr<MediaDownloader> CreateDownloader(
        const std::shared_ptr<MediaSource>& source, const SniffResult& result);
    std::shared_ptr<MediaDownloader> CreateDashDownloader();
    std::shared_ptr<MediaDownloader> CreateHlsDownloader();
    std::shared_ptr<MediaDownloader> CreateHttpDownloader(const std::shared_ptr<MediaSource>& source, bool isFlv);
    void InitSourceLoader(std::shared_ptr<MediaSource> source);
    void MediaStreamDfxTrace(std::shared_ptr<MediaSource> source);
    std::string GetCurUrl();
    Status InitSourcePlugin(const std::shared_ptr<MediaSource>& source);
    bool IsAllowedProtocol(std::string_view url);
    std::shared_ptr<MediaDownloader> GetDownloader() const;
    bool SetDownloader(std::shared_ptr<MediaDownloader> downloader);
    std::shared_ptr<SnifferCoordinator> GetActiveSniffer() const;
    bool SetActiveSniffer(std::shared_ptr<SnifferCoordinator> sniffer);
    void LogRoute(const char* event, const char* reason = "none", const char* phase = "route") const;
    uint32_t seekErrorCount_{0};
    std::atomic<uint64_t> routeId_ {0};
    std::weak_ptr<Callback> callback_;
    mutable std::mutex sourceStateMutex_;
    std::shared_ptr<ProbeFailureNotifier> probeFailure_;
    std::atomic<SourceState> sourceState_ {SourceState::IDLE};
    std::atomic<Status> routeStatus_ {Status::ERROR_UNKNOWN};
    std::shared_ptr<MediaDownloader> downloader_;
    mutable std::shared_mutex downloaderMutex_ {};
    bool delayReady_ {true};
    std::atomic<bool> sourceClosed_ {false};
    std::atomic<StreamProtocolType> protocolType_ {StreamProtocolType::HTTP};
    mutable std::shared_mutex activeSnifferMutex_;
    std::shared_ptr<SnifferCoordinator> activeSniffer_;
    std::string uri_ {};
    std::map<std::string, std::string> httpHeader_ {};
    std::string mimeType_ {};
    std::atomic<bool> isInterruptNeeded_{false};
    std::shared_ptr<MediaSourceLoaderCombinations> loaderCombinations_ {nullptr};
    std::shared_ptr<PlayStrategy> playStrategy_ {nullptr};
    std::string redirectUrl_ {};
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> reportInfo_ =
        std::make_shared<OHOS::MediaAVCodec::SourceStatisticsReportInfo>();
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif
