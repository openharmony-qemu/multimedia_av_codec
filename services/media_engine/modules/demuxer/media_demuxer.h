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

#ifndef MEDIA_DEMUXER_H
#define MEDIA_DEMUXER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>
#include <unordered_set>

#include "osal/task/condition_variable.h"
#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "common/seek_callback.h"
#include "demuxer/type_finder.h"
#include "demuxer/sample_queue.h"
#include "filter/filter.h"
#include "meta/media_types.h"
#include "osal/task/autolock.h"
#include "osal/task/task.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "plugin/demuxer_plugin.h"
#include "interrupt_listener.h"
#include "performance_utils.h"
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
class BaseStreamDemuxer;
class DemuxerPluginManager;
class Source;

using MediaSource = OHOS::Media::Plugins::MediaSource;
using MediaSyncManager = OHOS::Media::Pipeline::MediaSyncManager;
using FileType = OHOS::Media::Plugins::FileType;
using funcPreReadSample = std::function<int64_t(int32_t trackId)>;

class AVBufferQueueProducer;
enum class DemuxerTrackType : int32_t {
    VIDEO = 0,
    AUDIO,
    SUBTITLE,
};

class MediaDemuxer : public std::enable_shared_from_this<MediaDemuxer>,
                     public Plugins::Callback,
                     public InterruptListener,
                     public SampleQueueCallback {
public:
    explicit MediaDemuxer();
    ~MediaDemuxer() override;

    Status SetDataSource(const std::shared_ptr<MediaSource> &source);
    Status SetSubtitleSource(const std::shared_ptr<MediaSource> &source);
    void SetBundleName(const std::string& bundleName);
    Status SetOutputBufferQueue(int32_t trackId, const sptr<AVBufferQueueProducer>& producer);

    std::shared_ptr<Meta> GetGlobalMetaInfo();
    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo() const;
    std::shared_ptr<Meta> GetUserMeta();

    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);
    Status Reset();
    Status Start();
    Status Stop();
    Status Pause();
    Status PauseDragging();
    Status PauseAudioAlign();
    Status Resume();
    Status ResumeDragging();
    Status ResumeAudioAlign();
    Status Flush();
    Status Preroll();
    Status PausePreroll();

    Status StartTask(int32_t trackId);
    Status SelectTrack(uint32_t trackIndex); // Interface for AVDemuxer
    Status UnselectTrack(uint32_t trackIndex); // Interface for AVDemuxer
    Status ReadSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample); // Interface for AVDemuxer
    Status GetBitRates(std::vector<uint32_t> &bitRates);
    Status SelectBitRate(uint32_t bitRate, bool isAutoSelect = false);
    Status GetDownloadInfo(DownloadInfo& downloadInfo);
    Status GetPlaybackInfo(PlaybackInfo& playbackInfo);
    Status GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos);
    void SetDrmCallback(const std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> &callback);
    void HandleEvent(const Plugins::PluginEvent &event);
    void OnEvent(const Plugins::PluginEvent &event) override;
    void OnEventBuffer(const Plugins::PluginEvent &event, std::shared_ptr<Pipeline::EventReceiver> eventReceiver);
    void OnSeekReadyEvent(const Plugins::PluginEvent &event);
    void OnDashSeekReadyEvent(const Plugins::PluginEvent &event);
    void OnHlsSeekReadyEvent(const Plugins::PluginEvent &event);
    void OnDfxEvent(const Plugins::PluginDfxEvent &event) override;

    Status SetPerfRecEnabled(bool isPerfRecEnabled);

    std::map<int32_t, sptr<AVBufferQueueProducer>> GetBufferQueueProducerMap();
    Status PauseTaskByTrackId(int32_t trackId);
    bool IsRenderNextVideoFrameSupported();

    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver);
    bool GetDuration(int64_t& durationMs);
    void SetPlayerId(std::string playerId);
    void SetDumpInfo(bool isDump, uint64_t instanceId);

    Status OptimizeDecodeSlow(bool isDecodeOptimizationEnabled);
    Status SetDecoderFramerateUpperLimit(int32_t decoderFramerateUpperLimit, int32_t trackId);
    Status SetSpeed(float speed);
    Status SetFrameRate(double framerate, int32_t trackId);
    void OnInterrupted(bool isInterruptNeeded) override;
    void OnDumpInfo(int32_t fd);
    bool IsLocalDrmInfosExisted();
    Status DisableMediaTrack(Plugins::MediaType mediaType);
    void OnBufferAvailable(int32_t trackId);

    void SetSelectBitRateFlag(bool flag, uint32_t desBitRate) override;
    bool CanAutoSelectBitRate() override;

    bool IsRefParserSupported();
    Status StartReferenceParser(int64_t startTimeMs, bool isForward = true);
    Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo);
    Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo);
    Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo);
    bool IsVideoEos();
    bool HasEosTrack();
    inline bool IsAudioDemuxDecodeAsync() const
    {
        return isAudioDemuxDecodeAsync_;
    }
    Status GetIFramePos(std::vector<uint32_t> &IFramePos);
    Status Dts2FrameId(int64_t dts, uint32_t &frameId);
    Status SeekMs2FrameId(int64_t seekMs, uint32_t &frameId);
    Status FrameId2SeekMs(uint32_t frameId, int64_t &seekMs);
    void RegisterVideoStreamReadyCallback(const std::shared_ptr<VideoStreamReadyCallback> &callback);
    void DeregisterVideoStreamReadyCallback();

    Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index); // Interface for AVDemuxer
    Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs); // Interface for AVDemuxer
    Status ResumeDemuxerReadLoop();
    Status PauseDemuxerReadLoop();
    Status SetTranscoderMode();
    void SetCacheLimit(uint32_t limitSize);
    void SetEnableOnlineFdCache(bool isEnableFdCache);
    void WaitForBufferingEnd();
    int32_t GetCurrentVideoTrackId();
    int32_t GetTargetVideoTrackId(std::vector<std::shared_ptr<Meta>> trackInfos);
    void SetIsEnableReselectVideoTrack(bool isEnable);
    void SetApiVersion(int32_t apiVersion);
    bool IsLocalFd();
    Status RebootPlugin();
    uint64_t GetCachedDuration();
    void RestartAndClearBuffer();
    bool IsFlvLive();

    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);
    bool HasVideo();
    bool HasAudio();
    bool IsSeekToTimeSupported();

    void SetIsCreatedByFilter(bool isCreatedByFilter);

    Status GetCurrentCacheSize(uint32_t trackIndex, uint32_t& size); // Interface for AVDemuxer
    Status StopBufferring(bool isAppBackground);
    
    void SetMediaMuted(OHOS::Media::MediaType mediaType, bool isMuted, bool keepDecodingOnMute);
private:
    class AVBufferQueueProducerListener;
    class TrackWrapper;
    struct MediaMetaData {
        std::vector<std::shared_ptr<Meta>> trackMetas;
        std::shared_ptr<Meta> globalMeta;
    };

    struct MaintainBaseInfo {
        int64_t segmentOffset = -1;
        int64_t basePts = -1;
        int64_t candidateBasePts = -1;
        int64_t lastPts = 0;
        int64_t lastPtsModifyedMax = -1;
        bool isLastPtsChange = false;
    };

    struct SyncFrameInfo {
        int64_t pts = -1;
        int32_t skipOpenGopUnrefFrameCnt = 0;
    };

    bool isHttpSource_ = false;
    std::string videoMime_{};

    static constexpr int32_t TRACK_ID_INVALID = -1;
    static constexpr int32_t DEFAULT_DECODE_FRAMERATE_UPPER_LIMIT = 120;
    static inline bool IsValidTrackId(const int32_t trackId) { return trackId >= 0; }

    Status InnerPrepare();
    void InitMediaMetaData(const Plugins::MediaInfo& mediaInfo);
    void InitDefaultTrack(const Plugins::MediaInfo& mediaInfo, int32_t& videoTrackId,
        int32_t& audioTrackId, int32_t& subtitleTrackId, std::string& videoMime);
    bool IsOffsetValid(int64_t offset) const;
    std::shared_ptr<Meta> GetTrackMeta(int32_t trackId);
    Status AddDemuxerCopyTask(int32_t trackId, TaskType type);
    Status AddDemuxerCopyTaskByTrack(int32_t trackId, DemuxerTrackType type);
    void AddDemuxerCopyTaskByTrackIfFilter(int32_t trackId, DemuxerTrackType type);
    void AddDemuxerCopyTaskIfFilter(int32_t trackId, TaskType type);
    void AddHandleFlvSelectBitrateTask();

    void InitIsAudioDemuxDecodeAsync();
    Status StopAllTask();
    Status PauseAllTask();
    Status PauseAllTaskAsync();
    Status ResumeAllTask();
    void AccelerateTrackTask(int32_t trackId);
    void SetTrackNotifyFlag(int32_t trackId, bool isNotifyNeeded);
    void AccelerateSampleConsumerTask(int32_t trackId);
    void ResetInner();

    bool GetDrmInfosUpdated(const std::multimap<std::string, std::vector<uint8_t>> &newInfos,
        std::multimap<std::string, std::vector<uint8_t>> &result);
    Status ProcessDrmInfos();
    void HandleSourceDrmInfoEvent(const std::multimap<std::string, std::vector<uint8_t>> &info);
    Status ReportDrmInfos(const std::multimap<std::string, std::vector<uint8_t>> &info);

    void DumpBufferToFile(int32_t trackId, std::shared_ptr<AVBuffer> buffer);
    bool IsBufferDroppable(std::shared_ptr<AVBuffer> sample, int32_t trackId);
    bool CheckDropAudioFrame(std::shared_ptr<AVBuffer> sample, int32_t trackId);
    bool IsTrackDisabled(Plugins::MediaType mediaType);
    bool CheckTrackEnabledById(int32_t trackId);
    bool HandleDashChangeStream(int32_t trackId);

    void GetMemoryUsage(int32_t trackId, std::shared_ptr<Plugins::DemuxerPlugin> &pluginTemp);
    void ReportMemoryUsage(int32_t trackId, std::shared_ptr<Plugins::DemuxerPlugin> &pluginTemp);
    Status SeekToTimeAfter();
    bool SelectBitRateChangeStream(int32_t trackId);
    bool SelectTrackChangeStream(int32_t trackId);
    bool HandleSelectTrackChangeStream(int32_t trackId, int32_t newStreamID, int32_t& newTrackId);
    void HandleSelectTrackStreamSeek(int32_t streamID, int32_t& trackId);
    std::shared_ptr<Plugins::DemuxerPlugin> GetCurFFmpegPlugin();
    void UpdateThreadPriority(int32_t trackId);
    bool IsNeedMapToInnerTrackID();
    int64_t GetReadLoopRetryUs(int32_t trackId);
    uint64_t GetSampleQueueDuration();
    void UpdateSampleQueueCache();
    void ReportEosEvent();
    Plugins::Seekable seekable_;
    Plugins::Seekable subSeekable_;
    std::string uri_;
    std::string SubtitleUri_;
    uint64_t mediaDataSize_;
    uint64_t subMediaDataSize_;

    std::shared_ptr<MediaSource> mediaSource_;
    std::shared_ptr<Source> source_;
    std::shared_ptr<Source> subtitleSource_;
    MediaMetaData mediaMetaData_;

    int64_t DoBeforeEachLoop(int32_t trackId);
    int64_t DoBeforeSubtitleTrackReadLoop(int32_t trackId);
    int64_t ReadLoop(int32_t trackId);
    Status CopyFrameToUserQueue(int32_t trackId);
    bool GetBufferFromUserQueue(int32_t queueIndex, int32_t size = 0);
    Status InnerReadSample(int32_t trackId, std::shared_ptr<AVBuffer> sample, bool isAVDemuxer);
    Status InnerSelectTrack(int32_t trackId);
    Status HandleReadSample(int32_t trackId);
    int64_t ParserRefInfo();
    void TryReclaimParserTask();

    Status HandleSelectTrack(int32_t trackId);
    Status HandleDashSelectTrack(int32_t trackId);
    Status DoSelectTrack(int32_t trackId, int32_t curTrackId);
    Status HandleRebootPlugin(int32_t trackId, bool& isRebooted);
    Status HandleHlsRebootPlugin();
    Status HandleSeekChangeStream(int32_t currentStreamId, int32_t newStreamId, int32_t trackId);
    bool IsSubtitleMime(const std::string& mime);
    void HandleAutoMaintainPts(int32_t trackeId, std::shared_ptr<AVBuffer> sample);
    void InitPtsInfo();
    void InitMediaStartPts();
    void TranscoderInitMediaStartPts();
    void UpdateBufferQueueListener(int32_t trackId);
    bool IsOpenGopBufferDroppable(std::shared_ptr<AVBuffer> sample, int32_t trackId);
    void UpdateSyncFrameInfo(std::shared_ptr<AVBuffer> sample, int32_t trackId, bool isDiscardable = false);
    void EnterDraggingOpenGopCnt();
    void ResetDraggingOpenGopCnt();
    Status ReadSampleWithPerfRecord(const std::shared_ptr<Plugins::DemuxerPlugin> &pluginTemp,
        const int32_t &innerTrackID, const std::shared_ptr<AVBuffer> &sample, bool isAVDemuxer);
    Status HandleTrackEos(int32_t trackId);
    void SetOutputBufferPts(std::shared_ptr<AVBuffer> &outputBuffer);
    void TranscoderUpdateOutputBufferPts(int32_t trackId, std::shared_ptr<AVBuffer> &outputBuffer);

    Status AddSampleBufferQueue(int32_t trackId);
    int64_t SampleConsumerLoop(int32_t trackId);
    void SetTrackNotifySampleConsumerFlag(int32_t trackId, bool isNotifySampleConsumerNeeded);
    bool IsRightMediaTrack(int32_t trackId, DemuxerTrackType type) const;
    int64_t GetLastVideoBufferAbsPts(int32_t trackId) const;
    void UpdateLastVideoBufferAbsPts(int32_t trackId);
    std::string InferDemuxerPluginNameByContentType();
    bool IsHitPlugin(std::string& plugin, std::string& contentType);
    Status OnSelectBitrateOk(int64_t startPts, uint32_t bitRate) override;
    Status SelectBitrateForNonSQ(int64_t startPts, uint32_t bitRate);
    Status OnSampleQueueBufferAvailable(int32_t queueId) override;
    Status OnSampleQueueBufferConsume(int32_t queueId) override;
    Status NotifySampleQueueBufferConsume(int32_t queueId);
    Status HandleSelectBitrateForFlvLive(int64_t startPts, uint32_t bitrate);
    bool IsIgonreBuffering();
    void InitEnableSampleQueueFlag();
    inline bool GetEnableSampleQueueFlag() const
    {
        return enableSampleQueue_ && isAudioDemuxDecodeAsync_;
    }
    Status StartTaskWithSampleQueue(int32_t trackId);
    Status PushBufferToQueue(int32_t trackId, std::shared_ptr<AVBuffer>& buffer, bool available);
    void StartTaskInner(int32_t trackId);
    void InitAudioTrack();
    void InitVideoTrack();
    void InitSubtitleTrack();
    void HandlePacketConvertError();
    void HandleVideoTrack(int32_t trackId);
    Status HandlePushBuffer(int32_t trackId, std::shared_ptr<AVBuffer>& dstBuffer,
                            sptr<AVBufferQueueProducer>& bufferQueue, Status status);
    void HandleSeek(int32_t trackId);
    void RecordErrorCount(int32_t queueIndex, Status ret);
    std::atomic<bool> isFlvLiveSelectingBitRate_ = false;
    uint64_t demuxerCacheDuration_ = 0;
    uint64_t sourceCacheDuration_ = 0;
    int64_t lastClockTimeMs_ = 0;
    bool enableSampleQueue_ = true;

    Mutex mapMutex_{};
    std::map<int32_t, std::shared_ptr<TrackWrapper>> trackMap_;
    std::map<int32_t, sptr<AVBufferQueueProducer>> bufferQueueMap_;
    std::map<int32_t, std::shared_ptr<AVBuffer>> bufferMap_;

    // memoryUsage
    std::unordered_map<uint32_t, uint32_t> trackMemoryUsages_; // Event parameter
    std::unordered_map<int32_t, uint32_t> memoryReportLimitCount_;
    std::unordered_map<uint32_t, funcPreReadSample> funcBeforeReadSampleMap_;

    std::map<int32_t, std::shared_ptr<SampleQueue>> sampleQueueMap_;
    std::map<int32_t, bool> eosMap_;
    std::map<int32_t, uint32_t> requestBufferErrorCountMap_;
    std::atomic<bool> isThreadExit_ = true;
    bool useBufferQueue_ = false;
    bool isAccurateSeekForHLS_ = false;
    int64_t videoStartTime_{0};

    std::shared_mutex drmMutex{};
    std::mutex isSelectTrackMutex_{};
    std::mutex rebootPluginMutex_{};
    std::multimap<std::string, std::vector<uint8_t>> localDrmInfos_;
    std::shared_ptr<OHOS::MediaAVCodec::AVDemuxerCallback> drmCallback_;

    std::map<int32_t, std::unique_ptr<Task>> taskMap_;
    std::map<int32_t, std::unique_ptr<Task>> sampleConsumerTaskMap_;
    std::shared_ptr<MediaSyncManager> syncCenter_;
    bool isFlvLiveStream_ = false;
    std::unique_ptr<Task> handleFlvSelectBitrateTask_;
    std::unique_ptr<Task> notifySampleConsumeTask_;
    std::unique_ptr<Task> notifySampleProduceTask_;

    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_;
    int64_t lastSeekTime_{Plugins::HST_TIME_NONE};
    bool isSeeked_{false};
    int32_t videoTrackId_ {TRACK_ID_INVALID};
    int32_t audioTrackId_ {TRACK_ID_INVALID};
    int32_t subtitleTrackId_ {TRACK_ID_INVALID};
    bool firstAudio_{true};
    std::mutex stopMutex_ {};

    std::atomic<bool> isStopped_ = true;
    std::atomic<bool> isPaused_ = false;
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer_;
    std::shared_ptr<BaseStreamDemuxer> subStreamDemuxer_;
    std::string bundleName_ {};
    std::string playerId_;

    std::atomic<bool> isDecodeOptimizationEnabled_ {false};
    std::atomic<float> speed_ {1.0f};
    std::atomic<double> framerate_ {0.0};
    std::atomic<int32_t> decoderFramerateUpperLimit_ {DEFAULT_DECODE_FRAMERATE_UPPER_LIMIT};

    std::string subtitlePluginName_;
    std::shared_ptr<Plugins::DemuxerPlugin> subtitlePlugin_;
    std::shared_ptr<MediaSource> subtitleMediaSource_;
    bool isDump_ = false;
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager_;
    std::atomic<bool> isSelectBitRate_ = false;
    std::atomic<bool> isManualBitRateSetting_ = false;
    uint32_t targetBitRate_ = 0;
    std::string dumpPrefix_ = "";
    std::unordered_set<Plugins::MediaType> disabledMediaTracks_ {};

    std::unique_ptr<Task> parserRefInfoTask_;
    bool isFirstParser_ = true;
    bool isParserTaskEnd_ = false;
    bool isAudioDemuxDecodeAsync_ = true;
    bool isVideoTrackDisabled_ = true;
    std::mutex parserTaskMutex_ {};
    int64_t duration_ {0};
    FileType fileType_ = FileType::UNKNOW;

    std::mutex prerollMutex_ {};
    std::atomic<bool> inPreroll_ = false;

    std::map<int32_t, int32_t> inSelectTrackType_{};
    std::map<int32_t, std::pair<int32_t, int32_t>> seekReadyStreamInfo_{};
    std::condition_variable rebootPluginCondition_;
    std::atomic<bool> isSelectTrack_ = false;
    std::atomic<bool> shouldCheckAudioFramePts_ = false;
    int64_t lastAudioPts_ = 0;
    int64_t lastVideoPts_ = 0;
    int64_t lastAudioPtsInMute_ = 0;
    std::atomic<bool> isOnEventNoMemory_ = false;
    std::atomic<bool> isSeekError_ = false;
    std::atomic<bool> shouldCheckSubtitleFramePts_ = false;
    int64_t lastSubtitlePts_ = 0;
    std::shared_ptr<VideoStreamReadyCallback> VideoStreamReadyCallback_ = nullptr;
    std::mutex draggingMutex_ {};
    std::atomic<bool> isDemuxerLoopExecuting_ {false};
    std::atomic<bool> isFirstFrameAfterSeek_ {false};
    std::atomic<bool> isInterruptNeeded_ {false};
    bool isAutoMaintainPts_ = false;
    std::map<int32_t, std::shared_ptr<MaintainBaseInfo>> maintainBaseInfos_;
    int64_t mediaStartPts_ {HST_TIME_NONE};
    int64_t transcoderStartPts_ {HST_TIME_NONE};
    bool isEnableReselectVideoTrack_ {false};
    int32_t targetVideoTrackId_ {TRACK_ID_INVALID};
    SyncFrameInfo syncFrameInfo_ {};
    std::mutex syncFrameInfoMutex_ {};
    bool isTranscoderMode_ {false};
    bool perfRecEnabled_ { false };
    PerfRecorder perfRecorder_ {};
    int32_t apiVersion_ {0};
    bool isHlsFmp4_ {false};

    bool isCreatedByFilter_ {false};

    int64_t videoSeekTime_ {0};
    bool isInSeekDropAudio_ {false};
    std::atomic<int32_t> convertErrorTime_ {0};
    bool isVideoMuted_ = false;
    bool needReleaseVideoDecoder_ = false;
    bool needRestore_ {false};
    bool hasSetLargeSize_ {false};
    bool isNeedSetLarge_ {false};

    uint32_t timeout_ = {10}; // 10 represents 10ms. Optimization can consider dynamic adjustment.
    bool enableAsyncDemuxer_ = true;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H