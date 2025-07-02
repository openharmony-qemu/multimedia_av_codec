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

#ifndef MEDIA_PIPELINE_DEMUXER_FILTER_H
#define MEDIA_PIPELINE_DEMUXER_FILTER_H

#include <string>
#include <unordered_set>
#include "common/seek_callback.h"
#include "filter/filter.h"
#include "demuxer/media_demuxer.h"
#include "meta/meta.h"
#include "meta/media_types.h"
#include "osal/task/mutex.h"
#include "media_sync_manager.h"
#include "interrupt_monitor.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class DemuxerFilter : public Filter, public std::enable_shared_from_this<DemuxerFilter> {
public:
    explicit DemuxerFilter(std::string name, FilterType type);
    ~DemuxerFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;
    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback,
              const std::shared_ptr<InterruptMonitor>& monitor) override;
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoStop() override;
    Status DoPause() override;
    Status DoFreeze() override;
    Status DoUnFreeze() override;
    Status DoPauseDragging() override;
    Status DoPauseAudioAlign() override;
    Status DoResume() override;
    Status DoResumeDragging() override;
    Status DoResumeAudioAlign() override;
    Status DoFlush() override;
    Status DoPreroll() override;
    Status DoWaitPrerollDone(bool render) override;
    Status DoSetPerfRecEnabled(bool isPerfRecEnabled) override;
    Status Reset();
    Status PauseForSeek();
    Status ResumeForSeek();
    Status PrepareBeforeStart();

    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    void GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status SetTranscoderMode();
    Status SetSkippingAudioDecAndEnc();
    Status SetDataSource(const std::shared_ptr<MediaSource> source);
    Status SetSubtitleSource(const std::shared_ptr<MediaSource> source);
    void SetBundleName(const std::string& bundleName);
    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);

    bool IsRefParserSupported();
    Status StartReferenceParser(int64_t startTimeMs, bool isForward = true);
    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo);
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo);
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo);
    Status GetIFramePos(std::vector<uint32_t> &IFramePos);
    Status Dts2FrameId(int64_t dts, uint32_t &frameId);
    Status SeekMs2FrameId(int64_t seekMs, uint32_t &frameId);
    Status FrameId2SeekMs(uint32_t frameId, int64_t &seekMs);

    Status StartTask(int32_t trackId);
    Status SelectTrack(int32_t trackId);

    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;
    std::shared_ptr<Meta> GetGlobalMetaInfo() const;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status GetBitRates(std::vector<uint32_t>& bitRates);
    Status SelectBitRate(uint32_t bitRate, bool isAutoSelect = false);
    Status GetDownloadInfo(DownloadInfo& downloadInfo);
    Status GetPlaybackInfo(PlaybackInfo& playbackInfo);

    FilterType GetFilterType();

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    std::map<int32_t, sptr<AVBufferQueueProducer>> GetBufferQueueProducerMap();
    Status PauseTaskByTrackId(int32_t trackId);
    bool IsRenderNextVideoFrameSupported();

    inline bool IsAudioDemuxDecodeAsync() const
    {
        return demuxer_ && demuxer_->IsAudioDemuxDecodeAsync();
    }

    bool IsDrmProtected();
    // drm callback
    void OnDrmInfoUpdated(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo);
    bool GetDuration(int64_t& durationMs);
    Status OptimizeDecodeSlow(bool isDecodeOptimizationEnabled);
    Status SetSpeed(float speed);
    void SetDumpFlag(bool isdump);
    void OnDumpInfo(int32_t fd);
    void SetCallerInfo(uint64_t instanceId, const std::string& appName);
    bool IsVideoEos();
    bool HasEosTrack();
    Status DisableMediaTrack(Plugins::MediaType mediaType);
    void RegisterVideoStreamReadyCallback(const std::shared_ptr<VideoStreamReadyCallback> &callback);
    void DeregisterVideoStreamReadyCallback();
    Status ResumeDemuxerReadLoop();
    Status PauseDemuxerReadLoop();
    void WaitForBufferingEnd();
    int32_t GetCurrentVideoTrackId();

    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);
    void SetIsNotPrepareBeforeStart(bool isNotPrepareBeforeStart);
    void SetIsEnableReselectVideoTrack(bool isEnable);
    void SetApiVersion(int32_t apiVersion);
    bool IsLocalFd();
    Status RebootPlugin();
    uint64_t GetCachedDuration();
    void RestartAndClearBuffer();
    bool IsFlvLive();
    Status StopBufferring(bool isAppBackground);
    Status SetMediaMuted(OHOS::Media::MediaType mediaType, bool isMuted, bool keepDecodingOnMute);
    void HandleDecoderErrorFrame(int64_t pts);
    bool IsVideoMuted();
protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    bool FindTrackId(StreamType outType, int32_t &trackId);
    bool FindStreamType(StreamType &streamType, Plugins::MediaType mediaType, std::string mime,
        size_t index, std::shared_ptr<Meta> &meta);
    bool ShouldTrackSkipped(Plugins::MediaType mediaType, std::string mime, size_t index);
    void UpdateTrackIdMap(StreamType streamType, int32_t index);
    Status FaultDemuxerEventInfoWrite(StreamType& streamType);
    bool IsVideoMime(const std::string& mime);
    bool IsAudioMime(const std::string& mime);
    bool CheckIsBigendian(std::shared_ptr<Meta> &meta);
    Status HandleTrackInfos(const std::vector<std::shared_ptr<Meta>> &trackInfos, int32_t &successNodeCount);
    std::string CollectVideoAndAudioMime();
    std::string uri_;
    std::atomic<bool> isLoopStarted{false};

    std::shared_ptr<Filter> nextFilter_;
    std::shared_ptr<MediaDemuxer> demuxer_;
    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    bool isTransCoderMode_ {false};

    std::map<StreamType, std::vector<int32_t>> track_id_map_;
    Mutex mapMutex_ {};

    bool isDump_ = false;
    std::string bundleName_;
    uint64_t instanceId_ = 0;
    std::string videoMime_;
    std::string audioMime_;
    std::unordered_set<Plugins::MediaType> disabledMediaTracks_ {};
    bool isNotPrepareBeforeStart_ {true};
    bool isEnableReselectVideoTrack_ {false};
    int32_t apiVersion_ {0};
    bool isVideoMuted_ = false;
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_DEMUXER_FILTER_H