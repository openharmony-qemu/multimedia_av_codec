/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include "media_downloader.h"
#include "meta/media_types.h"
#include "plugin/source_plugin.h"
#include "download/media_source_loading_request.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpSourcePlugin : public SourcePlugin {
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
    Status GetStreamInfo(std::vector<StreamInfo>& streams) override;
    Status SelectStream(int32_t streamID) override;
    void SetDemuxerState(int32_t streamId) override;
    void SetDownloadErrorState() override;
    void SetInterruptState(bool isInterruptNeeded) override;
    Status GetDownloadInfo(DownloadInfo& downloadInfo) override;
    Status SetCurrentBitRate(int32_t bitRate, int32_t streamID) override;
    Status GetPlaybackInfo(PlaybackInfo& playbackInfo) override;
    size_t GetSegmentOffset() override;
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

private:
    void CloseUri(bool isAsync = false);
    std::shared_ptr<PlayStrategy> PlayStrategyInit(std::shared_ptr<MediaSource> source);
    void SetDownloaderBySource(std::shared_ptr<MediaSource> source);
    bool CheckIsM3U8Uri();
    void InitHttpSource(const std::shared_ptr<MediaSource>& source);
    void MediaStreamDfxTrace(std::shared_ptr<MediaSource> source);
    std::string GetCurUrl();
    Status InitSourcePlugin(const std::shared_ptr<MediaSource>& source);
    bool IsDash();
    uint32_t bufferSize_;
    uint32_t waterline_;
    uint32_t seekErrorCount_{0};
    std::weak_ptr<Callback> callback_;
    std::shared_ptr<MediaDownloader> downloader_;
    Mutex mutex_ {};
    bool delayReady_ {true};
    std::string uri_ {};
    std::map<std::string, std::string> httpHeader_ {};
    std::string mimeType_ {};
    std::atomic<bool> isInterruptNeeded_{false};
    std::shared_ptr<MediaSourceLoaderCombinations> loaderCombinations_ {nullptr};
    std::string redirectUrl_ {};
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif