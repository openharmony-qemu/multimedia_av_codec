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

#ifndef MEDIA_PIPELINE_VIDEO_SINK_FILTER_H
#define MEDIA_PIPELINE_VIDEO_SINK_FILTER_H

#include <atomic>
#include "plugin/plugin_time.h"
#include "avcodec_common.h"
#include "surface/surface.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"
#include "osal/task/task.h"
#include "video_sink.h"
#include "sink/media_synchronous_sink.h"
#include "common/status.h"
#include "video_decoder_adapter.h"
#include "meta/meta.h"
#include "meta/format.h"
#include "filter/filter.h"
#include "media_sync_manager.h"
#include "common/media_core.h"
#include "common/seek_callback.h"
#include "drm_i_keysession_service.h"
#include "interrupt_listener.h"
#include "sei_parser_helper.h"
#include "post_processor/base_video_post_processor.h"
#include "post_processor/video_post_processor_factory.h"
#include "common/fdsan_fd.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class DecoderSurfaceFilter : public Filter, public std::enable_shared_from_this<DecoderSurfaceFilter>,
    public InterruptListener {
public:
    explicit DecoderSurfaceFilter(const std::string& name, FilterType type);
    ~DecoderSurfaceFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    Status Configure(const std::shared_ptr<Meta> &parameter);
    Status DoInitAfterLink() override;
    Status DoPrepare() override;
    Status DoStart() override;
    Status DoPause() override;
    Status DoFreeze() override;
    Status DoUnFreeze() override;
    Status DoPauseDragging() override;
    Status DoResume() override;
    Status DoResumeDragging() override;
    Status DoStop() override;
    Status DoFlush() override;
    Status DoRelease() override;
    Status DoPreroll() override;
    Status DoWaitPrerollDone(bool render) override;
    Status DoSetPlayRange(int64_t start, int64_t end) override;
    Status DoProcessInputBuffer(int recvArg, bool dropFrame) override;
    Status DoProcessOutputBuffer(int recvArg, bool dropFrame, bool byIdx, uint32_t idx, int64_t renderTime) override;
    Status DoSetPerfRecEnabled(bool isPerfRecEnabled) override;

    void SetParameter(const std::shared_ptr<Meta>& parameter) override;
    void GetParameter(std::shared_ptr<Meta>& parameter) override;
    void OnInterrupted(bool isInterruptNeeded) override;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    FilterType GetFilterType();
    void DrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    void DecoderDrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    Status SetVideoSurface(sptr<Surface> videoSurface);

    Status SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp);

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode);
    void PostProcessorOnError(int32_t errorCode);

    sptr<AVBufferQueueProducer> GetInputBufferQueue();
    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);
    void SetSeekTime(int64_t seekTimeUs, PlayerSeekMode mode = PlayerSeekMode::SEEK_CLOSEST);
    void ResetSeekInfo();
    Status HandleInputBuffer();
    void OnDumpInfo(int32_t fd);

    void SetCallingInfo(int32_t appUid, int32_t appPid, std::string bundleName, uint64_t instanceId);

    Status GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration);
    void SetBitrateStart();
    void OnOutputFormatChanged(const MediaAVCodec::Format &format);
    Status StartSeekContinous();
    Status StopSeekContinous();
    void RegisterVideoFrameReadyCallback(std::shared_ptr<VideoFrameReadyCallback> &callback);
    void DeregisterVideoFrameReadyCallback();
    int32_t GetDecRateUpperLimit();
    bool GetIsSupportSeekWithoutFlush();
    void ConsumeVideoFrame(uint32_t index, bool isRender, int64_t renderTimeNs = 0L);
    Status SetSeiMessageCbStatus(bool status, const std::vector<int32_t> &payloadTypes);

    Status InitPostProcessor();
    void SetPostProcessorType(VideoPostProcessorType type);
    Status SetPostProcessorOn(bool isSuperResolutionOn);
    Status SetVideoWindowSize(int32_t width, int32_t height);
    void NotifyAudioComplete();
    Status SetSpeed(float speed);
    Status SetPostProcessorFd(int32_t postProcessorFd);
    Status SetCameraPostprocessing(bool enable);
    Status SetCameraPostprocessingDirect(bool enable);
    void NotifyPause();
    void NotifyMemoryExchange(bool exchangeFlag);
    Status SetMediaMuted(bool isMuted, bool hasInitialized);
    Status DoReleaseOnMuted(bool isNeedRelease) override;
    Status DoReInitAndStart() override;
    void SetIsSwDecoder(bool isSwDecoder);
    void SetBuffering(bool isBuffering);

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    void RenderLoop();
    std::string GetCodecName(std::string mimeType);
    int64_t CalculateNextRender(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer, int64_t& actionClock);
    void ParseDecodeRateLimit();
    void RenderNextOutput(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    void HandleRender(int index, bool render, const std::shared_ptr<AVBuffer>& outBuffer, int64_t& renderTime);
    void WriteDfxTimeToVector(int64_t currentRenderPts, int64_t currentSysTimeNs,
        int64_t lastRenderTimeNs, int64_t frameIntervalNs, std::vector<int64_t>& timeStampList);
    void WriteDfxTimeToVector(int64_t syncEndMs, int64_t redererStartMs, int64_t framePtsMs,
        int64_t lastRenderTimeMs, int64_t frameIntervalMs, std::vector<int64_t>& timeStampList);
    Status ReleaseOutputBuffer(int index, bool render, const std::shared_ptr<AVBuffer> &outBuffer, int64_t renderTime);
    void DoReleaseOutputBuffer(uint32_t index, bool render, int64_t pts = 0);
    void DoRenderOutputBufferAtTime(uint32_t index, int64_t renderTime, int64_t pts = 0);
    bool AcquireNextRenderBuffer(bool byIdx, uint32_t &index, std::shared_ptr<AVBuffer> &outBuffer,
        int64_t renderTime = 0);
    bool DrainSeekContinuous(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    bool DrainPreroll(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    bool DrainSeekClosest(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    void HandleFirstOutput();
    void HandleEosOutput(int index);
    void ReportEosEvent();
    void RenderAtTimeDfx(int64_t renderTimeNs, int64_t currentTimeNs, int64_t lastRenderTimeNs);
    int64_t GetSystimeTimeNs();
    bool IsPostProcessorSupported();
    Status HwDecoderToSwDecoder(std::string codecMimeType);
    std::shared_ptr<BaseVideoPostProcessor> CreatePostProcessor();
    void InitPostProcessorType();
#ifdef SUPPORT_CAMERA_POST_PROCESSOR
    void LoadCameraPostProcessorLib();
#endif
    Status CheckBufferDecodedCorrectly(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    std::string GetMime();

    std::string name_;
    FilterType filterType_;
    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<VideoDecoderAdapter> videoDecoder_;
    std::shared_ptr<VideoSink> videoSink_;
    std::string codecMimeType_;
    std::shared_ptr<Meta> configureParameter_;
    std::shared_ptr<Meta> meta_;

    std::shared_ptr<Filter> nextFilter_;

    Format configFormat_;

    std::atomic<uint64_t> renderFrameCnt_{0};
    std::atomic<uint64_t> discardFrameCnt_{0};
    std::atomic<bool> isSeek_{false};
    int64_t seekTimeUs_{0};

    bool refreshTotalPauseTime_{false};
    int64_t latestBufferTime_{HST_TIME_NONE};
    int64_t latestPausedTime_{HST_TIME_NONE};
    int64_t totalPausedTime_{0};
    int64_t stopTime_{0};
    sptr<Surface> decoderOutputSurface_;
    sptr<Surface> videoSurface_;
    bool isSwDecoder_ = false;
    bool hasToSwDecoder_ = false;
    bool hasReceiveUnsupportError_ = false;
    bool isDrmProtected_ = false;
#ifdef SUPPORT_DRM
    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy_ = nullptr;
#endif
    bool svpFlag_ = false;
    std::atomic<bool> isPaused_{false};
    std::list<std::pair<uint32_t, std::shared_ptr<AVBuffer>>> outputBuffers_;
    std::mutex mutex_;
    std::unique_ptr<std::thread> readThread_ = nullptr;
    std::atomic<bool> isThreadExit_ = true;
    std::condition_variable condBufferAvailable_;

    std::atomic<bool> isRenderStarted_{false};
    std::atomic<bool> isInterruptNeeded_{false};
    Mutex formatChangeMutex_{};
    int32_t rateUpperLimit_{0};

    std::mutex prerollMutex_ {};
    std::atomic<bool> inPreroll_ {false};
    std::condition_variable prerollDoneCond_ {};
    std::atomic<bool> prerollDone_ {true};
    std::atomic<bool> eosNext_ {false};
    bool isFirstFrameAfterResume_ {true};

    int32_t appUid_ = -1;
    int32_t appPid_ = -1;
    std::string bundleName_;
    uint64_t instanceId_ = 0;
    int64_t playRangeStartTime_ = -1;
    int64_t playRangeEndTime_ = -1;

    std::atomic<int32_t> bitrateChange_{0};
    int32_t surfaceWidth_{0};
    int32_t surfaceHeight_{0};

    std::shared_ptr<VideoFrameReadyCallback> videoFrameReadyCallback_;
    bool isInSeekContinous_{false};
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> outputBufferMap_;
    std::mutex draggingMutex_ {};
    std::unique_ptr<Task> eosTask_ {nullptr};
    std::atomic<int64_t> lastRenderTimeNs_ = HST_TIME_NONE;
    int64_t renderTimeMaxAdvanceUs_ { 80000 };
    bool enableRenderAtTime_ {true};
    bool enableRenderAtTimeDfx_ {false};
    std::list<int64_t> renderTimeQueue_;
    std::string logMessage;
    bool seiMessageCbStatus_ {false};
    std::vector<int32_t> payloadTypes_ {};
    sptr<SeiParserListener> producerListener_ {};
    sptr<Media::AVBufferQueueProducer> inputBufferQueueProducer_ {};

    static constexpr int32_t DEFAULT_TARGET_WIDTH = 1920;
    static constexpr int32_t DEFAULT_TARGET_HEIGHT = 1080;
    int32_t postProcessorTargetWidth_ {DEFAULT_TARGET_WIDTH};
    int32_t postProcessorTargetHeight_ {DEFAULT_TARGET_HEIGHT};
    bool isPostProcessorOn_ {false};
    bool isPostProcessorSupported_ {true};
    VideoPostProcessorType postProcessorType_ { VideoPostProcessorType::NONE };
    std::shared_ptr<BaseVideoPostProcessor> postProcessor_;
    std::atomic<bool> enableCameraPostprocessing_ {false};
    bool isCameraPostProcessorSupported_ {true};
 
    int64_t eosPts_ {INT64_MAX};
    int64_t prevDecoderPts_ {INT64_MAX};
    std::mutex fdMutex_ {};
    std::unique_ptr<FdsanFd> fdsanFd_ = nullptr;
    int32_t preScaleType_ {0};
#ifdef SUPPORT_CAMERA_POST_PROCESSOR
    std::mutex loadLibMutex_ {};
    static void *cameraPostProcessorLibHandle_;
#endif

    std::atomic<bool> isVideoMuted_ {false};
    bool isDecoderReleasedForMute_ {true};
    bool hasReceivedReleaseEvent_ {false};
    bool isFirstStart_ = true;
    bool isBuffering_ {false};
    bool isFirstFrameWrite_ {false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H
