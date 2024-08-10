/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include "media_demuxer.h"
#include "meta/meta.h"
#include "meta/media_types.h"
#include "osal/task/mutex.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class DemuxerFilter : public Filter, public std::enable_shared_from_this<DemuxerFilter> {
public:
    explicit DemuxerFilter(std::string name, FilterType type);
    ~DemuxerFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;
    Status DoPrepare() override;
    Status DoPrepareFrame(bool renderFirstFrame) override;
    Status DoStart() override;
    Status DoStop() override;
    Status DoPause() override;
    Status DoResume() override;
    Status DoResumeDragging() override;
    Status DoFlush() override;
    Status Reset();
    Status PauseForSeek();
    Status ResumeForSeek();
    Status PrepareBeforeStart();

    void SetParameter(const std::shared_ptr<Meta> &parameter) override;
    void GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status SetDataSource(const std::shared_ptr<MediaSource> source);
    Status SetSubtitleSource(const std::shared_ptr<MediaSource> source);
    void SetBundleName(const std::string& bundleName);
    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);

    Status StartReferenceParser(int64_t startTimeMs, bool isForward = true);
    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo);
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo);
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo);
    Status GetIFramePos(std::vector<uint32_t> &IFramePos);
    Status Dts2FrameId(int64_t dts, uint32_t &frameId, bool offset = true);

    Status StartTask(int32_t trackId);
    Status SelectTrack(int32_t trackId);

    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;
    std::shared_ptr<Meta> GetGlobalMetaInfo() const;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status GetBitRates(std::vector<uint32_t>& bitRates);
    Status SelectBitRate(uint32_t bitRate);
    Status GetDownloadInfo(DownloadInfo& downloadInfo);
    Status GetPlaybackInfo(PlaybackInfo& playbackInfo);

    FilterType GetFilterType();

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);
    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    std::map<uint32_t, sptr<AVBufferQueueProducer>> GetBufferQueueProducerMap();
    Status PauseTaskByTrackId(int32_t trackId);
    bool IsRenderNextVideoFrameSupported();

    bool IsDrmProtected();
    // drm callback
    void OnDrmInfoUpdated(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo);
    bool GetDuration(int64_t& durationMs);
    Status OptimizeDecodeSlow(bool isDecodeOptimizationEnabled);
    Status SetSpeed(float speed);
    void SetInterruptState(bool isInterruptNeeded);
    void SetDumpFlag(bool isdump);
    void OnDumpInfo(int32_t fd);
    void SetCallerInfo(uint64_t instanceId, const std::string& appName);
    bool IsVideoEos();
    Status DisableMediaTrack(Plugins::MediaType mediaType);
    void RegisterVideoStreamReadyCallback(const std::shared_ptr<VideoStreamReadyCallback> &callback);
    void DeregisterVideoStreamReadyCallback();
    Status ResumeDemuxerReadLoop();
    Status PauseDemuxerReadLoop();
protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    struct MediaMetaData {
        std::vector<std::shared_ptr<Meta>> trackMetas;
        std::shared_ptr<Meta> globalMeta;
    };

    bool FindTrackId(StreamType outType, int32_t &trackId);
    bool FindStreamType(StreamType &streamType, Plugins::MediaType mediaType, std::string mime, size_t index);
    bool ShouldTrackSkipped(Plugins::MediaType mediaType, std::string mime, size_t index);
    void UpdateTrackIdMap(StreamType streamType, int32_t index);
    void FaultDemuxerEventInfoWrite(StreamType& streamType);
    bool IsVideoMime(const std::string& mime);
    bool IsAudioMime(const std::string& mime);
    Status HandleTrackInfos(const std::vector<std::shared_ptr<Meta>> &trackInfos, int32_t &successNodeCount);
    std::string CollectVideoAndAudioMime();
    std::string uri_;
    std::atomic<bool> isLoopStarted{false};
    std::atomic<bool> isPrepareFramed{false};

    std::shared_ptr<Filter> nextFilter_;
    MediaMetaData mediaMetaData_;
    std::shared_ptr<MediaDemuxer> demuxer_;
    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;

    std::map<StreamType, std::vector<int32_t>> track_id_map_;
    Mutex mapMutex_ {};

    bool isDump_ = false;
    std::string bundleName_;
    uint64_t instanceId_ = 0;
    std::string videoMime_;
    std::string audioMime_;
    std::unordered_set<Plugins::MediaType> disabledMediaTracks_ {};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_DEMUXER_FILTER_H