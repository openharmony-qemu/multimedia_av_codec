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

#define MEDIA_PIPELINE

#include "decoder_surface_filter.h"
#include <sys/time.h>
#include "filter/filter_factory.h"
#include "plugin/plugin_time.h"
#include "avcodec_errors.h"
#include "common/log.h"
#include "common/media_core.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "avcodec_list.h"
#include "osal/utils/util.h"
#include "parameters.h"
#include "scope_guard.h"
#include "scoped_timer.h"
#ifdef SUPPORT_DRM
#include "imedia_key_session_service.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "DecoderSurfaceFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
static const int64_t PLAY_RANGE_DEFAULT_VALUE = -1;
static const int64_t MICROSECONDS_CONVERT_UNIT = 1000; // ms change to us
static const int64_t NS_PER_US = 1000; // us change to ns
static const uint32_t PREROLL_WAIT_TIME = 1000; // Lock wait for 1000ms.
static const uint32_t TASK_DELAY_TOLERANCE = 5 * 1000 * 1000; // task delay tolerance 5_000_000ns also 5ms
static const int64_t MAX_DEBUG_LOG = 10;
static const int32_t MAX_ADVANCE_US = 80000; // max advance us at render time
static const int64_t OVERTIME_WARNING_MS = 50;
static const double DEFAULT_FRAME_RATE = 30.0; // 30.0 is the hisi default frame rate.
static const std::string ENHANCE_FLAG = "com.openharmony.deferredVideoEnhanceFlag";
static const std::string VIDEO_ID = "com.openharmony.videoId";

static AutoRegisterFilter<DecoderSurfaceFilter> g_registerDecoderSurfaceFilter("builtin.player.videodecoder",
    FilterType::FILTERTYPE_VDEC, [](const std::string& name, const FilterType type) {
        return std::make_shared<DecoderSurfaceFilter>(name, FilterType::FILTERTYPE_VDEC);
    });

static const bool IS_FILTER_ASYNC = system::GetParameter("persist.media_service.async_filter", "1") == "1";

static const std::string VIDEO_INPUT_BUFFER_QUEUE_NAME = "VideoDecoderInputBufferQueue";

class DecoderSurfaceFilterLinkCallback : public FilterLinkCallback {
public:
    explicit DecoderSurfaceFilterLinkCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter) {}

    ~DecoderSurfaceFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        MEDIA_LOG_I("OnLinkedResult");
        decoderSurfaceFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        decoderSurfaceFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        decoderSurfaceFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

const std::unordered_map<VideoScaleType, OHOS::ScalingMode> SCALEMODE_MAP = {
    { VideoScaleType::VIDEO_SCALE_TYPE_FIT, OHOS::SCALING_MODE_SCALE_TO_WINDOW },
    { VideoScaleType::VIDEO_SCALE_TYPE_FIT_CROP, OHOS::SCALING_MODE_SCALE_CROP},
    { VideoScaleType::VIDEO_SCALE_TYPE_SCALED_ASPECT, OHOS::SCALING_MODE_SCALE_FIT},
};

class FilterVideoPostProcessorCallback : public PostProcessorCallback {
public:
    explicit FilterVideoPostProcessorCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter) {}

    ~FilterVideoPostProcessorCallback() = default;

    void OnError(int32_t errorCode) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->PostProcessorOnError(errorCode);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

    void OnOutputFormatChanged(const Format &format) override
    {
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->DrainOutputBuffer(index, buffer);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }
private:
    std::weak_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

class FilterMediaCodecCallbackWithPostProcessor : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit FilterMediaCodecCallbackWithPostProcessor(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter) {}

    ~FilterMediaCodecCallbackWithPostProcessor() = default;

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->OnError(errorType, errorCode);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->OnOutputFormatChanged(format);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->DecoderDrainOutputBuffer(index, buffer);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

private:
    std::weak_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

class FilterMediaCodecCallback : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit FilterMediaCodecCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter) {}

    ~FilterMediaCodecCallback() = default;

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        auto decoderSurfaceFilter = decoderSurfaceFilter_.lock();
        FALSE_RETURN_MSG(decoderSurfaceFilter != nullptr, "invalid decoderSurfaceFilter");
        decoderSurfaceFilter->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
        auto decoderSurfaceFilter = decoderSurfaceFilter_.lock();
        FALSE_RETURN_MSG(decoderSurfaceFilter != nullptr, "invalid decoderSurfaceFilter");
        decoderSurfaceFilter->OnOutputFormatChanged(format);
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        auto decoderSurfaceFilter = decoderSurfaceFilter_.lock();
        FALSE_RETURN_MSG(decoderSurfaceFilter != nullptr, "invalid decoderSurfaceFilter");
        decoderSurfaceFilter->DrainOutputBuffer(index, buffer);
    }

private:
    std::weak_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};


class AVBufferAvailableListener : public OHOS::Media::IConsumerListener {
public:
    explicit AVBufferAvailableListener(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
    {
        decoderSurfaceFilter_ = decoderSurfaceFilter;
    }
    ~AVBufferAvailableListener() = default;

    void OnBufferAvailable()
    {
        auto decoderSurfaceFilter = decoderSurfaceFilter_.lock();
        FALSE_RETURN_MSG(decoderSurfaceFilter != nullptr, "invalid decoderSurfaceFilter");
        decoderSurfaceFilter->HandleInputBuffer();
    }

private:
    std::weak_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

DecoderSurfaceFilter::DecoderSurfaceFilter(const std::string& name, FilterType type)
    : Filter(name, type, IS_FILTER_ASYNC)
{
    videoDecoder_ = std::make_shared<VideoDecoderAdapter>();
    videoSink_ = std::make_shared<VideoSink>();
    filterType_ = type;
    enableRenderAtTime_ = system::GetParameter("debug.media_service.enable_renderattime", "1") == "1";
    renderTimeMaxAdvanceUs_ = static_cast<int64_t>
        (system::GetIntParameter("debug.media_service.renderattime_advance", MAX_ADVANCE_US));
    enableRenderAtTimeDfx_ = system::GetParameter("debug.media_service.enable_renderattime_dfx", "0") == "1";
}

DecoderSurfaceFilter::~DecoderSurfaceFilter()
{
    MEDIA_LOG_I("~DecoderSurfaceFilter()");
    Filter::StopFilterTask();
    ON_SCOPE_EXIT(0) {
        DoRelease();
        MEDIA_LOG_I("~DecoderSurfaceFilter() exit.");
    };
    FALSE_RETURN(!IS_FILTER_ASYNC && !isThreadExit_);
    isThreadExit_ = true;
    condBufferAvailable_.notify_all();
    FALSE_RETURN(readThread_ != nullptr && readThread_->joinable());
    readThread_->join();
    readThread_ = nullptr;
}

void DecoderSurfaceFilter::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOG_E("AVCodec error happened. ErrorType: %{public}d, errorCode: %{public}d",
        static_cast<int32_t>(errorType), errorCode);
    FALSE_RETURN(eventReceiver_ != nullptr);
    eventReceiver_->OnEvent({"DecoderSurfaceFilter", EventType::EVENT_ERROR, MSERR_EXT_API9_IO});
}

void DecoderSurfaceFilter::PostProcessorOnError(int32_t errorCode)
{
    MEDIA_LOG_E("Post processor error happened. ErrorCode: %{public}d", errorCode);
    FALSE_RETURN(eventReceiver_ != nullptr);
    eventReceiver_->OnEvent({"DecoderSurfaceFilter", EventType::EVENT_ERROR, MSERR_EXT_API9_IO});
}

void DecoderSurfaceFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    videoSink_->SetEventReceiver(eventReceiver_);
    FALSE_RETURN(videoDecoder_ != nullptr);
    videoDecoder_->SetEventReceiver(eventReceiver_);
}

Status DecoderSurfaceFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    configFormat_.SetMeta(configureParameter_);
    Status ret = videoDecoder_->Configure(configFormat_);

    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback = nullptr;
    if (postProcessor_ != nullptr) {
        mediaCodecCallback = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(shared_from_this());
        std::shared_ptr<PostProcessorCallback> postProcessorCallback
            = std::make_shared<FilterVideoPostProcessorCallback>(shared_from_this());
        postProcessor_->SetCallback(postProcessorCallback);
    } else {
        mediaCodecCallback = std::make_shared<FilterMediaCodecCallback>(shared_from_this());
    }
    videoDecoder_->SetCallback(mediaCodecCallback);
    return ret;
}

Status DecoderSurfaceFilter::DoInitAfterLink()
{
    Status ret;
    // create secure decoder for drm.
    MEDIA_LOG_I("DoInit enter the codecMimeType_ is %{public}s", codecMimeType_.c_str());
    if (videoDecoder_ != nullptr) {
        videoDecoder_->SetCallingInfo(appUid_, appPid_, bundleName_, instanceId_);
    } else {
        return Status::ERROR_UNKNOWN;
    }
    videoDecoder_->SetCallingInfo(appUid_, appPid_, bundleName_, instanceId_);
    if (isDrmProtected_ && svpFlag_) {
        MEDIA_LOG_D("DecoderSurfaceFilter will create secure decoder for drm-protected videos");
        std::string baseName = GetCodecName(codecMimeType_);
        FALSE_RETURN_V_MSG(!baseName.empty(),
            Status::ERROR_INVALID_PARAMETER, "get name by mime failed.");
        std::string secureDecoderName = baseName + ".secure";
        MEDIA_LOG_D("DecoderSurfaceFilter will create secure decoder %{public}s", secureDecoderName.c_str());
        ScopedTimer timer("drm-protected VideoDecoder Init", OVERTIME_WARNING_MS);
        ret = videoDecoder_->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, false, secureDecoderName);
    } else {
        ScopedTimer timer("VideoDecoder Init", OVERTIME_WARNING_MS);
        ret = videoDecoder_->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, codecMimeType_);
    }

    if (ret != Status::OK && eventReceiver_ != nullptr) {
        MEDIA_LOG_E("Init decoder fail ret = %{public}d", ret);
        eventReceiver_->OnEvent({"decoderSurface", EventType::EVENT_ERROR, MSERR_UNSUPPORT_VID_DEC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }

    ret = Configure(meta_);
    if (ret != Status::OK) {
        eventReceiver_->OnEvent({"decoderSurface", EventType::EVENT_ERROR, MSERR_UNSUPPORT_VID_SRC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    ParseDecodeRateLimit();
    videoDecoder_->SetOutputSurface(decoderOutputSurface_);
    if (isDrmProtected_) {
#ifdef SUPPORT_DRM
        videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
#endif
    }

    ret = InitPostProcessor();
    FALSE_RETURN_V(ret == Status::OK, ret);

    videoSink_->SetParameter(meta_);
    eosTask_ = std::make_unique<Task>("OS_EOSv", groupId_, TaskType::VIDEO, TaskPriority::HIGH, false);
    return Status::OK;
}

Status DecoderSurfaceFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    if (onLinkedResultCallback_ != nullptr) {
        videoDecoder_->PrepareInputBufferQueue();
        sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
        sptr<Media::AVBufferQueueConsumer> inputBufferQueueConsumer = videoDecoder_->GetBufferQueueConsumer();
        FALSE_RETURN_V_MSG(inputBufferQueueConsumer != nullptr, Status::ERROR_NULL_POINTER,
                           "inputBufferQueueConsumer_ is nullptr");
        inputBufferQueueConsumer->SetBufferAvailableListener(listener);
        onLinkedResultCallback_->OnLinkedResult(videoDecoder_->GetBufferQueueProducer(), meta_);
        if (seiMessageCbStatus_) {
            inputBufferQueueProducer_ = videoDecoder_->GetBufferQueueProducer();
        }
    }
    isRenderStarted_ = false;
    return Status::OK;
}

Status DecoderSurfaceFilter::HandleInputBuffer()
{
    ProcessInputBuffer();
    return Status::OK;
}

Status DecoderSurfaceFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    if (isPaused_.load()) {
        MEDIA_LOG_I("DoStart after pause to execute resume.");
        return DoResume();
    }
    if (!IS_FILTER_ASYNC) {
        isThreadExit_ = false;
        isPaused_ = false;
        readThread_ = std::make_unique<std::thread>(&DecoderSurfaceFilter::RenderLoop, this);
        pthread_setname_np(readThread_->native_handle(), "RenderLoop");
    }
    auto ret = videoDecoder_->Start();
    FALSE_RETURN_V(ret == Status::OK, ret);
    if (postProcessor_) {
        ret = postProcessor_->Start();
    }
    return ret;
}

Status DecoderSurfaceFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    isPaused_ = true;
    isFirstFrameAfterResume_ = false;
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_all();
    }
    videoSink_->ResetSyncInfo();
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

void DecoderSurfaceFilter::NotifyAudioComplete()
{
    FALSE_RETURN(videoSink_ != nullptr);
    videoSink_->ResetSyncInfo();
}

Status DecoderSurfaceFilter::DoPauseDragging()
{
    MEDIA_LOG_I("DoPauseDragging enter.");
    DoPause();
    lastRenderTimeNs_ = GetSystimeTimeNs();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!outputBufferMap_.empty()) {
            MEDIA_LOG_E("DoPauseDragging outputBufferMap_ size = %{public}zu", outputBufferMap_.size());
            for (auto it = outputBufferMap_.begin(); it != outputBufferMap_.end(); ++it) {
                DoReleaseOutputBuffer(it->first, false);
            }
            outputBufferMap_.clear();
        }
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    refreshTotalPauseTime_ = true;
    isPaused_ = false;
    isFirstFrameAfterResume_ = true;
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_all();
    }
    videoDecoder_->Start();
    if (postProcessor_) {
        postProcessor_->Start();
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoResumeDragging()
{
    MEDIA_LOG_I("DoResumeDragging enter.");
    refreshTotalPauseTime_ = true;
    isPaused_ = false;
    lastRenderTimeNs_ = HST_TIME_NONE;
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_all();
    }
    videoDecoder_->Start();
    if (postProcessor_) {
        postProcessor_->Start();
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    latestBufferTime_ = HST_TIME_NONE;
    latestPausedTime_ = HST_TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    isPaused_ = false;
    lastRenderTimeNs_ = HST_TIME_NONE;
    playRangeStartTime_ = PLAY_RANGE_DEFAULT_VALUE;
    playRangeEndTime_ = PLAY_RANGE_DEFAULT_VALUE;

    timeval tv;
    gettimeofday(&tv, 0);
    stopTime_ = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec; // 1000000 means transfering from s to us.
    videoSink_->ResetSyncInfo();
    auto ret = videoDecoder_->Stop();
    Status ret2 = Status::OK;
    if (postProcessor_) {
        ret2 = postProcessor_->Stop();
    }
    if (!IS_FILTER_ASYNC && !isThreadExit_.load()) {
        isThreadExit_ = true;
        condBufferAvailable_.notify_all();
        FALSE_RETURN_V(readThread_ != nullptr && readThread_->joinable(), ret);
        readThread_->join();
        readThread_ = nullptr;
    }
    FALSE_RETURN_V(ret == Status::OK, ret);
    return ret2;
}

Status DecoderSurfaceFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    lastRenderTimeNs_ = HST_TIME_NONE;
    eosPts_ = INT64_MAX;
    videoDecoder_->Flush();
    if (postProcessor_) {
        postProcessor_->Flush();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        MEDIA_LOG_I("Flush");
        outputBuffers_.clear();
        outputBufferMap_.clear();
    }
    videoSink_->ResetSyncInfo();
    return Status::OK;
}

Status DecoderSurfaceFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    videoDecoder_->Release();
    if (postProcessor_) {
        postProcessor_->Release();
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoPreroll()
{
    MEDIA_LOG_I("DoPreroll enter.");
    std::lock_guard<std::mutex> lock(prerollMutex_);
    Status ret = Status::OK;
    FALSE_RETURN_V_MSG(!inPreroll_.load(), Status::OK, "DoPreroll in preroll now.");
    inPreroll_.store(true);
    prerollDone_.store(false);
    eosNext_.store(false);
    if (isPaused_.load()) {
        ret = DoResume();
    } else {
        ret = DoStart();
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("video decoder start failed, ret = %{public}d", ret);
        eventReceiver_->OnEvent({"decoderSurface", EventType::EVENT_ERROR,
            MSERR_VID_DEC_FAILED});
    }
    Filter::StartFilterTask();
    return ret;
}

Status DecoderSurfaceFilter::DoWaitPrerollDone(bool render)
{
    MEDIA_LOG_I("DoWaitPrerollDone enter.");
    std::unique_lock<std::mutex> lock(prerollMutex_);
    FALSE_RETURN_V(inPreroll_.load(), Status::OK);
    if (prerollDone_.load() || isInterruptNeeded_.load()) {
        MEDIA_LOG_I("Receive preroll frame before DoWaitPrerollDone.");
    } else {
        prerollDoneCond_.wait_for(lock, std::chrono::milliseconds(PREROLL_WAIT_TIME),
            [this] () { return prerollDone_.load() || isInterruptNeeded_.load(); });
    }
    Filter::PauseFilterTask();
    DoPause();
    if (!prerollDone_.load()) {
        MEDIA_LOG_E("No preroll frame received!");
        inPreroll_.store(false);
        prerollDone_.store(true);
        eosNext_.store(false);
        return Status::OK;
    }
    std::unique_lock<std::mutex> bufferLock(mutex_);
    if (render && !eosNext_.load() && !outputBuffers_.empty()) {
        std::pair<int, std::shared_ptr<AVBuffer>> nextTask = std::move(outputBuffers_.front());
        outputBuffers_.pop_front();
        DoReleaseOutputBuffer(nextTask.first, true);
    }
    eosNext_.store(false);
    if (!outputBuffers_.empty()) {
        Filter::ProcessOutputBuffer(1, 0);
    }
    inPreroll_.store(false);
    return Status::OK;
}

Status DecoderSurfaceFilter::DoSetPerfRecEnabled(bool isPerfRecEnabled)
{
    isPerfRecEnabled_ = isPerfRecEnabled;
    Status res = Status::OK;
    if (videoSink_ != nullptr) {
        res = videoSink_->SetPerfRecEnabled(isPerfRecEnabled);
    }
    if (videoDecoder_ != nullptr) {
        res = videoDecoder_->SetPerfRecEnabled(isPerfRecEnabled);
    }
    return res;
}

Status DecoderSurfaceFilter::DoSetPlayRange(int64_t start, int64_t end)
{
    MEDIA_LOG_I("DoSetPlayRange");
    playRangeStartTime_ = start;
    playRangeEndTime_ = end;
    return Status::OK;
}

static OHOS::ScalingMode ConvertMediaScaleType(VideoScaleType scaleType)
{
    if (SCALEMODE_MAP.find(scaleType) == SCALEMODE_MAP.end()) {
        return OHOS::SCALING_MODE_SCALE_CROP;
    }
    return SCALEMODE_MAP.at(scaleType);
}

void DecoderSurfaceFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter %{public}i", parameter != nullptr);
    Format format;
    if (parameter->Find(Tag::VIDEO_SCALE_TYPE) != parameter->end()) {
        int32_t scaleType;
        parameter->Get<Tag::VIDEO_SCALE_TYPE>(scaleType);
        preScaleType_ = scaleType;
        OHOS::ScalingMode scalingMode = ConvertMediaScaleType(static_cast<VideoScaleType>(scaleType));
        if (videoSurface_) {
            GSError err = videoSurface_->SetScalingMode(scalingMode);
            if (err != GSERROR_OK) {
                MEDIA_LOG_W("set ScalingMode %{public}d to surface failed", static_cast<int>(scalingMode));
                return;
            }
            MEDIA_LOG_D("set ScalingMode %{public}d to surface success", static_cast<int>(scalingMode));
        }
        parameter->Remove(Tag::VIDEO_SCALE_TYPE);
    }
    if (parameter->Find(Tag::VIDEO_FRAME_RATE) != parameter->end()) {
        double rate = 0.0;
        parameter->Get<Tag::VIDEO_FRAME_RATE>(rate);
        if (rate < 0) {
            if (configFormat_.GetDoubleValue(Tag::VIDEO_FRAME_RATE, rate)) {
                MEDIA_LOG_W("rate is invalid, get frame rate from the original resource: %{public}f", rate);
            }
        }
        if (rate <= 0) {
            rate = DEFAULT_FRAME_RATE;
        }
        format.PutDoubleValue(Tag::VIDEO_FRAME_RATE, rate);
    }
    // cannot set parameter when codec at [ CONFIGURED / INITIALIZED ] state
    auto ret = videoDecoder_->SetParameter(format);
    if (ret == MediaAVCodec::AVCS_ERR_INVALID_STATE) {
        MEDIA_LOG_W("SetParameter at invalid state");
        videoDecoder_->Reset();
        if (!IS_FILTER_ASYNC && !isThreadExit_.load()) {
            DoStop();
        }
        videoDecoder_->Configure(configFormat_);
        videoDecoder_->SetOutputSurface(decoderOutputSurface_);
        if (isDrmProtected_) {
#ifdef SUPPORT_DRM
            videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
#endif
        }
    }
    videoDecoder_->SetParameter(format);
    if (postProcessor_) {
        postProcessor_->SetParameter(format);
    }
}

Status DecoderSurfaceFilter::GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration)
{
    FALSE_RETURN_V(videoSink_ != nullptr, Status::ERROR_INVALID_OPERATION);
    videoSink_->GetLagInfo(lagTimes, maxLagDuration, avgLagDuration);
    return Status::OK;
}

void DecoderSurfaceFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter enter parameter is valid:  %{public}i", parameter != nullptr);
}

void DecoderSurfaceFilter::SetCallingInfo(int32_t appUid, int32_t appPid, std::string bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}

void DecoderSurfaceFilter::OnInterrupted(bool isInterruptNeeded)
{
    MEDIA_LOG_D("DecoderSurfaceFilter onInterrupted %{public}d", isInterruptNeeded);
    std::lock_guard<std::mutex> lock(prerollMutex_);
    isInterruptNeeded_ = isInterruptNeeded;
    prerollDoneCond_.notify_all();
}

Status DecoderSurfaceFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext enter, nextFilter is valid:  %{public}i, outType: %{public}u",
        nextFilter != nullptr, static_cast<uint32_t>(outType));
    return Status::OK;
}

Status DecoderSurfaceFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status DecoderSurfaceFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

FilterType DecoderSurfaceFilter::GetFilterType()
{
    return filterType_;
}

std::string DecoderSurfaceFilter::GetCodecName(std::string mimeType)
{
    MEDIA_LOG_I("GetCodecName.");
    std::string codecName;
    auto codeclist = MediaAVCodec::AVCodecListFactory::CreateAVCodecList();
    FALSE_RETURN_V_MSG_E(codeclist != nullptr, codecName, "GetCodecName failed due to codeclist nullptr.");
    MediaAVCodec::Format format;
    format.PutStringValue("codec_mime", mimeType);
    codecName = codeclist->FindDecoder(format);
    return codecName;
}

bool DecoderSurfaceFilter::IsPostProcessorSupported()
{
    MEDIA_LOG_D("IsPostProcessorSupported enter.");
    return VideoPostProcessorFactory::Instance().IsPostProcessorSupported(postProcessorType_, meta_);
}

void DecoderSurfaceFilter::InitPostProcessorType()
{
    FALSE_RETURN_NOLOG(postProcessorType_ == VideoPostProcessorType::NONE && meta_ != nullptr);
    std::string enhanceflag;
    meta_->GetData(ENHANCE_FLAG, enhanceflag);
    MEDIA_LOG_D("enhanceflag: %{public}s", enhanceflag.c_str());
    FALSE_RETURN_NOLOG(
        enableCameraPostprocessing_.load() && enhanceflag == "1" && fdsanFd_ != nullptr && fdsanFd_->Get() >= 0);
    postProcessorType_ = VideoPostProcessorType::CAMERA_INSERT_FRAME;
    std::string videoId;
    meta_->GetData(VIDEO_ID, videoId);
    MEDIA_LOG_D("videoId: %{public}s", videoId.c_str());
    FALSE_RETURN_NOLOG(!videoId.empty());
    configFormat_.PutStringValue(VIDEO_ID, videoId);
}

Status DecoderSurfaceFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    meta_ = meta;
    FALSE_RETURN_V_MSG(meta->GetData(Tag::MIME_TYPE, codecMimeType_),
        Status::ERROR_INVALID_PARAMETER, "get mime failed.");

    meta_->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, isDrmProtected_);
    if (isCameraPostProcessorSupported_) {
        InitPostProcessorType();
    }
    isPostProcessorSupported_ = IsPostProcessorSupported();
    if (!isPostProcessorSupported_ || CreatePostProcessor() == nullptr) {
        isPostProcessorSupported_ = false;
        if (postProcessorType_ == VideoPostProcessorType::SUPER_RESOLUTION) {
            eventReceiver_->OnEvent({"SuperResolutionPostProcessor", EventType::EVENT_SUPER_RESOLUTION_CHANGED, false});
        }
    }
    onLinkedResultCallback_ = callback;
    return Filter::OnLinked(inType, meta, callback);
}

Status DecoderSurfaceFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    return Status::OK;
}

Status DecoderSurfaceFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Status::OK;
}

void DecoderSurfaceFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult");
}

void DecoderSurfaceFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
}

void DecoderSurfaceFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
}

Status DecoderSurfaceFilter::DoProcessOutputBuffer(int recvArg, bool dropFrame, bool byIdx, uint32_t idx,
                                                   int64_t renderTime)
{
    MEDIA_LOG_D("DoProcessOutputBuffer idx " PUBLIC_LOG_U32 " renderTime " PUBLIC_LOG_D64, idx, renderTime);
    FALSE_RETURN_V(!dropFrame, Status::OK);
    uint32_t index = idx;
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    bool acquireRes = AcquireNextRenderBuffer(byIdx, index, outputBuffer, renderTime);
    FALSE_RETURN_V(acquireRes, Status::OK);
    ReleaseOutputBuffer(index, recvArg, outputBuffer, renderTime);
    return Status::OK;
}

bool DecoderSurfaceFilter::AcquireNextRenderBuffer(bool byIdx, uint32_t &index, std::shared_ptr<AVBuffer> &outBuffer,
                                                   int64_t renderTime)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!byIdx) {
        FALSE_RETURN_V(!outputBuffers_.empty(), false);
        std::pair<int, std::shared_ptr<AVBuffer>> task = std::move(outputBuffers_.front());
        outputBuffers_.pop_front();
        FALSE_RETURN_V(task.first >= 0, false);
        index = static_cast<uint32_t>(task.first);
        outBuffer = task.second;
        if (isFirstFrameAfterResume_) {
            int64_t curTimeNs = GetSystimeTimeNs();
            videoSink_->UpdateTimeAnchorActually(outBuffer,
                (renderTime > 0 && renderTime > curTimeNs) ? renderTime - curTimeNs : 0);
            isFirstFrameAfterResume_ = false;
        }
        if (!outputBuffers_.empty()) {
            std::pair<int, std::shared_ptr<AVBuffer>> nextTask = outputBuffers_.front();
            RenderNextOutput(nextTask.first, nextTask.second);
        }
        return true;
    }
    FALSE_RETURN_V(outputBufferMap_.find(index) != outputBufferMap_.end(), false);
    outBuffer = outputBufferMap_[index];
    outputBufferMap_.erase(index);
    return true;
}

Status DecoderSurfaceFilter::ReleaseOutputBuffer(int index, bool render, const std::shared_ptr<AVBuffer> &outBuffer,
                                                 int64_t renderTime)
{
    if (!isRenderStarted_.load() && render && !(outBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS))
        && !isInSeekContinous_) {
        HandleFirstOutput();
    }
    if ((playRangeEndTime_ != PLAY_RANGE_DEFAULT_VALUE) &&
        (outBuffer->pts_ > playRangeEndTime_ * MICROSECONDS_CONVERT_UNIT)) {
        MEDIA_LOG_I("ReleaseBuffer for eos, SetPlayRange start: " PUBLIC_LOG_D64 ", end: " PUBLIC_LOG_D64,
                    playRangeStartTime_, playRangeEndTime_);
        HandleEosOutput(index);
        return Status::OK;
    }
 
    if ((outBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS)) && !isInSeekContinous_) {
        ResetSeekInfo();
        MEDIA_LOG_I("ReleaseBuffer for eos, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, outBuffer->GetUniqueId(),
                outBuffer->pts_, outBuffer->flag_);
        HandleEosOutput(index);
        return Status::OK;
    }

    if (outBuffer->pts_ < 0) {
        MEDIA_LOG_W("Avoid render video frame with pts=" PUBLIC_LOG_D64, outBuffer->pts_);
        DoReleaseOutputBuffer(index, false);
        return Status::OK;
    }

    if (renderTime > 0L && render) {
        int64_t currentSysTimeNs = GetSystimeTimeNs();
        int64_t lastRenderTimeNs = lastRenderTimeNs_.load();
        int64_t minRendererTime = std::max(currentSysTimeNs, lastRenderTimeNs == HST_TIME_NONE ? 0 : lastRenderTimeNs);
        renderTime = renderTime < minRendererTime ? minRendererTime : renderTime;
        MEDIA_LOG_D("ReleaseOutputBuffer index " PUBLIC_LOG_U32 " renderTime " PUBLIC_LOG_D64 " curTime " PUBLIC_LOG_D64
            " lastRenderTimeNs " PUBLIC_LOG_D64, index, renderTime, currentSysTimeNs, lastRenderTimeNs);
        RenderAtTimeDfx(renderTime, currentSysTimeNs, lastRenderTimeNs);
        DoRenderOutputBufferAtTime(index, renderTime, outBuffer->pts_);
        if (!isInSeekContinous_) {
            lastRenderTimeNs_ = renderTime;
        }
    } else {
        MEDIA_LOG_D("ReleaseOutputBuffer index= " PUBLIC_LOG_D32" isRender= " PUBLIC_LOG_U32" pts= " PUBLIC_LOG_D64,
                    index, static_cast<uint32_t>(render), outBuffer->pts_);
        DoReleaseOutputBuffer(index, render, outBuffer->pts_);
    }
    if (!isInSeekContinous_) {
        int64_t renderDelay = renderTime > 0L && render ?
            (renderTime - GetSystimeTimeNs()) / MICROSECONDS_CONVERT_UNIT : 0;
        videoSink_->SetLastPts(outBuffer->pts_, renderDelay);
    }
    return Status::OK;
}

void DecoderSurfaceFilter::DoReleaseOutputBuffer(uint32_t index, bool render, int64_t pts)
{
    if (postProcessor_) {
        postProcessor_->ReleaseOutputBuffer(index, render);
    } else if (videoDecoder_) {
        videoDecoder_->ReleaseOutputBuffer(index, render, pts);
    }
}

void DecoderSurfaceFilter::DoRenderOutputBufferAtTime(uint32_t index, int64_t renderTime, int64_t pts)
{
    if (postProcessor_) {
        postProcessor_->RenderOutputBufferAtTime(index, renderTime);
    } else if (videoDecoder_) {
        videoDecoder_->RenderOutputBufferAtTime(index, renderTime, pts);
    }
}

Status DecoderSurfaceFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    videoDecoder_->AquireAvailableInputBuffer();
    return Status::OK;
}

int64_t DecoderSurfaceFilter::CalculateNextRender(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    (void) index;
    int64_t waitTime = -1;
    MEDIA_LOG_D("DrainOutputBuffer not seeking and render. pts: " PUBLIC_LOG_D64, outputBuffer->pts_);
    videoSink_->SetFirstPts(outputBuffer->pts_);
    waitTime = videoSink_->DoSyncWrite(outputBuffer);
    return waitTime;
}

// async filter should call this function
void DecoderSurfaceFilter::RenderNextOutput(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (isInSeekContinous_) {
        Filter::ProcessOutputBuffer(false, 0);
        return;
    }
    
    int64_t waitTime = CalculateNextRender(index, outputBuffer);
    if (enableRenderAtTime_) {
        int64_t renderTimeNs = waitTime * NS_PER_US + GetSystimeTimeNs();
        MEDIA_LOG_D("RenderNextOutput enter. pts: " PUBLIC_LOG_D64 "  waitTime: " PUBLIC_LOG_D64
                    "  renderTimeNs: " PUBLIC_LOG_D64, outputBuffer->pts_, waitTime, renderTimeNs);
        // most renderTimeMaxAdvanceUs_ in advance
        Filter::ProcessOutputBuffer(waitTime >= 0,
            waitTime > renderTimeMaxAdvanceUs_ ? waitTime - renderTimeMaxAdvanceUs_ : 0, false, 0, renderTimeNs);
        return;
    }
    MEDIA_LOG_D("RenderNextOutput enter. pts: " PUBLIC_LOG_D64"  waitTime: " PUBLIC_LOG_D64,
        outputBuffer->pts_, waitTime);
    Filter::ProcessOutputBuffer(waitTime >= 0, waitTime);
}

void DecoderSurfaceFilter::ConsumeVideoFrame(uint32_t index, bool isRender, int64_t renderTimeNs)
{
    MEDIA_LOG_D("ConsumeVideoFrame idx " PUBLIC_LOG_U32 " renderTimeNs " PUBLIC_LOG_D64, index, renderTimeNs);
    Filter::ProcessOutputBuffer(isRender, 0, true, index, renderTimeNs);
}

void DecoderSurfaceFilter::DrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    MEDIA_LOG_D("DrainOutputBuffer, pts:" PUBLIC_LOG_D64, outputBuffer->pts_);
    if ((outputBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS))) {
        MEDIA_LOG_I("DrainOutputBuffer output EOS");
    }
    std::unique_lock<std::mutex> lock(mutex_);
    FALSE_RETURN_NOLOG(!DrainSeekContinuous(index, outputBuffer));
    FALSE_RETURN_NOLOG(postProcessor_ != nullptr || !DrainSeekClosest(index, outputBuffer));
    FALSE_RETURN_NOLOG(!DrainPreroll(index, outputBuffer));
    if (IS_FILTER_ASYNC && outputBuffers_.empty()) {
        RenderNextOutput(index, outputBuffer);
    }
    outputBuffers_.push_back(make_pair(index, outputBuffer));
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_one();
    }
}

void DecoderSurfaceFilter::DecoderDrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    MEDIA_LOG_D("DecoderDrainOutputBuffer pts: " PUBLIC_LOG_D64, outputBuffer->pts_);
    if (outputBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS)) {
        MEDIA_LOG_I("Decoder output EOS");
        eosPts_ = prevDecoderPts_;
        if (postProcessor_ != nullptr) {
            postProcessor_->NotifyEos(eosPts_);
        }
    }
    prevDecoderPts_ = outputBuffer->pts_;
    FALSE_RETURN_NOLOG(!DrainSeekClosest(index, outputBuffer));
    videoDecoder_->ReleaseOutputBuffer(index, true, outputBuffer->pts_);
}

void DecoderSurfaceFilter::RenderLoop()
{
    while (true) {
        std::pair<int, std::shared_ptr<AVBuffer>> nextTask;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condBufferAvailable_.wait(lock, [this] {
                return (!outputBuffers_.empty() && !isPaused_.load()) || isThreadExit_.load();
            });
            if (isThreadExit_) {
                MEDIA_LOG_I("Exit RenderLoop read thread.");
                break;
            }
            nextTask = std::move(outputBuffers_.front());
            outputBuffers_.pop_front();
        }
        int64_t waitTime = CalculateNextRender(nextTask.first, nextTask.second);
        MEDIA_LOG_D("RenderLoop pts: " PUBLIC_LOG_D64"  waitTime:" PUBLIC_LOG_D64,
            nextTask.second->pts_, waitTime);
        if (waitTime > 0) {
            OSAL::SleepFor(waitTime / 1000); // 1000 convert to ms
        }
        ReleaseOutputBuffer(nextTask.first, waitTime >= 0, nextTask.second, -1);
    }
}

bool DecoderSurfaceFilter::DrainSeekContinuous(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    FALSE_RETURN_V_NOLOG(isInSeekContinous_, false);
    bool isEOS = outputBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    FALSE_RETURN_V_NOLOG(!isEOS, false);
    outputBufferMap_.insert(std::make_pair(index, outputBuffer));
    std::shared_ptr<VideoFrameReadyCallback> videoFrameReadyCallback = nullptr;
    {
        std::unique_lock<std::mutex> draggingLock(draggingMutex_);
        FALSE_RETURN_V_NOLOG(videoFrameReadyCallback_ != nullptr, false);
        MEDIA_LOG_D("[drag_debug]DrainSeekContinuous dts: " PUBLIC_LOG_D64 ", pts: " PUBLIC_LOG_D64
            " bufferIdx: " PUBLIC_LOG_D32,
            outputBuffer->dts_, outputBuffer->pts_, index);
        videoFrameReadyCallback = videoFrameReadyCallback_;
    }
    FALSE_RETURN_V_NOLOG(videoFrameReadyCallback != nullptr, false);
    videoFrameReadyCallback->ConsumeVideoFrame(outputBuffer, index);
    return true;
}

bool DecoderSurfaceFilter::DrainPreroll(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    FALSE_RETURN_V_NOLOG(inPreroll_.load(), false);
    if (prerollDone_.load()) {
        outputBuffers_.push_back(make_pair(index, outputBuffer));
        return true;
    }
    std::lock_guard<std::mutex> lock(prerollMutex_);
    FALSE_RETURN_V_NOLOG(inPreroll_.load() && !prerollDone_.load(), false);
    outputBuffers_.push_back(make_pair(index, outputBuffer));
    bool isEOS = outputBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    eosNext_.store(isEOS);
    prerollDone_.store(true);
    MEDIA_LOG_I("receive preroll output, pts: " PUBLIC_LOG_D64 " bufferIdx: " PUBLIC_LOG_D32,
        outputBuffer->pts_, index);
    prerollDoneCond_.notify_all();
    return true;
}

bool DecoderSurfaceFilter::DrainSeekClosest(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    FALSE_RETURN_V_NOLOG(isSeek_, false);
    bool isEOS = outputBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    if (outputBuffer->pts_ < seekTimeUs_ && !isEOS) {
        videoDecoder_->ReleaseOutputBuffer(index, false, outputBuffer->pts_);
        return true;
    }
    MEDIA_LOG_I("Seek arrive target video pts: " PUBLIC_LOG_D64, seekTimeUs_);
    isSeek_ = false;
    return false;
}

Status DecoderSurfaceFilter::SetVideoSurface(sptr<Surface> videoSurface)
{
    if (!videoSurface) {
        MEDIA_LOG_W("videoSurface is null");
        return Status::ERROR_INVALID_PARAMETER;
    }
    videoSurface_ = videoSurface;
    OHOS::ScalingMode scalingMode = ConvertMediaScaleType(static_cast<VideoScaleType>(preScaleType_));
    GSError err = videoSurface_->SetScalingMode(scalingMode);
    if (err != GSERROR_OK) {
        MEDIA_LOG_W("set ScalingMode %{public}d to surface failed", static_cast<int>(scalingMode));
    }
    MEDIA_LOG_D("set ScalingMode %{public}d to surface success", static_cast<int>(scalingMode));
    if (postProcessor_ != nullptr) {
        MEDIA_LOG_I("postProcessor_ SetOutputSurface in");
        Status res = postProcessor_->SetOutputSurface(videoSurface_);
        if (res != Status::OK) {
            MEDIA_LOG_E(" postProcessor_ SetOutputSurface error, result is " PUBLIC_LOG_D32, res);
            return Status::ERROR_UNKNOWN;
        }
    } else {
        decoderOutputSurface_ = videoSurface;
    }

    if (videoDecoder_ != nullptr) {
        MEDIA_LOG_I("videoDecoder_ SetOutputSurface in");
        int32_t res = videoDecoder_->SetOutputSurface(decoderOutputSurface_);
        if (res != OHOS::MediaAVCodec::AVCodecServiceErrCode::AVCS_ERR_OK) {
            MEDIA_LOG_E("videoDecoder_ SetOutputSurface error, result is " PUBLIC_LOG_D32, res);
            return Status::ERROR_UNKNOWN;
        }
    }
    MEDIA_LOG_I("SetVideoSurface success");
    return Status::OK;
}

void DecoderSurfaceFilter::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter)
{
    MEDIA_LOG_I("SetSyncCenter enter");
    FALSE_RETURN(videoSink_ != nullptr);
    videoSink_->SetSyncCenter(syncCenter);
}

Status DecoderSurfaceFilter::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MEDIA_LOG_I("SetDecryptConfig");
    if (keySessionProxy == nullptr) {
        MEDIA_LOG_E("SetDecryptConfig keySessionProxy is nullptr.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    isDrmProtected_ = true;
    svpFlag_ = svp;
#ifdef SUPPORT_DRM
    keySessionServiceProxy_ = keySessionProxy;
#endif
    return Status::OK;
}

void DecoderSurfaceFilter::SetSeekTime(int64_t seekTimeUs, PlayerSeekMode mode)
{
    MEDIA_LOG_I("SetSeekTime");
    if (mode == PlayerSeekMode::SEEK_CLOSEST) {
        isSeek_ = true;
        seekTimeUs_ = seekTimeUs;
    }
    FALSE_RETURN_NOLOG(postProcessor_ != nullptr);
    postProcessor_->SetSeekTime(seekTimeUs, mode);
}

void DecoderSurfaceFilter::ResetSeekInfo()
{
    MEDIA_LOG_I("ResetSeekInfo");
    isSeek_ = false;
    seekTimeUs_ = 0;
    FALSE_RETURN_NOLOG(postProcessor_ != nullptr);
    postProcessor_->ResetSeekInfo();
}

void DecoderSurfaceFilter::ParseDecodeRateLimit()
{
    MEDIA_LOG_I("ParseDecodeRateLimit entered.");
    std::shared_ptr<MediaAVCodec::AVCodecList> codecList = MediaAVCodec::AVCodecListFactory::CreateAVCodecList();
    if (codecList == nullptr) {
        MEDIA_LOG_E("create avcodeclist failed.");
        return;
    }
    int32_t height = 0;
    bool ret = meta_->GetData(Tag::VIDEO_HEIGHT, height);
    FALSE_RETURN_MSG(ret || height <= 0, "failed to get video height");
    int32_t width = 0;
    ret = meta_->GetData(Tag::VIDEO_WIDTH, width);
    FALSE_RETURN_MSG(ret || width <= 0, "failed to get video width");

    MediaAVCodec::CapabilityData *capabilityData = codecList->GetCapability(codecMimeType_, false,
        MediaAVCodec::AVCodecCategory::AVCODEC_NONE);
    std::shared_ptr<MediaAVCodec::VideoCaps> videoCap = std::make_shared<MediaAVCodec::VideoCaps>(capabilityData);
    FALSE_RETURN_MSG(videoCap != nullptr, "failed to get videoCap instance");
    const MediaAVCodec::Range &frameRange = videoCap->GetSupportedFrameRatesFor(width, height);
    rateUpperLimit_ = frameRange.maxVal;
    if (rateUpperLimit_ > 0) {
        meta_->SetData(Tag::VIDEO_DECODER_RATE_UPPER_LIMIT, rateUpperLimit_);
    }
}

int32_t DecoderSurfaceFilter::GetDecRateUpperLimit()
{
    return rateUpperLimit_;
}

bool DecoderSurfaceFilter::GetIsSupportSeekWithoutFlush()
{
    std::shared_ptr<MediaAVCodec::AVCodecList> codecList = MediaAVCodec::AVCodecListFactory::CreateAVCodecList();
    FALSE_RETURN_V_MSG_E(codecList != nullptr, false, "create avcodeclist failed");
    MediaAVCodec::CapabilityData *capabilityData = codecList->GetCapability(codecMimeType_, false,
        MediaAVCodec::AVCodecCategory::AVCODEC_NONE);
    std::shared_ptr<MediaAVCodec::AVCodecInfo> videoCap = std::make_shared<MediaAVCodec::AVCodecInfo>(capabilityData);
    FALSE_RETURN_V_MSG_E(videoCap != nullptr, false, "failed to get videoCap instance");
    return videoCap->IsFeatureSupported(MediaAVCodec::AVCapabilityFeature::VIDEO_DECODER_SEEK_WITHOUT_FLUSH);
}

void DecoderSurfaceFilter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("DecoderSurfaceFilter::OnDumpInfo called.");
    FALSE_RETURN(videoDecoder_ != nullptr);
    videoDecoder_->OnDumpInfo(fd);
}

void DecoderSurfaceFilter::SetBitrateStart()
{
    bitrateChange_++;
}
 
void DecoderSurfaceFilter::OnOutputFormatChanged(const MediaAVCodec::Format &format)
{
    AutoLock lock(formatChangeMutex_);
    int32_t width = 0;
    format.GetIntValue("video_picture_width", width);
    int32_t height = 0;
    format.GetIntValue("video_picture_height", height);
    if (width <= 0 || height <= 0) {
        MEDIA_LOG_W("invaild video size");
        return;
    }
    if (surfaceWidth_ == 0 || surfaceWidth_ == 0) {
        MEDIA_LOG_I("receive first output Format");
        surfaceWidth_ = width;
        surfaceHeight_ = height;
        return;
    }
    if (surfaceWidth_ == width && surfaceHeight_ == height) {
        MEDIA_LOG_W("receive the same output Format");
        return;
    } else {
        MEDIA_LOG_I("OnOutputFormatChanged curW=" PUBLIC_LOG_D32 " curH=" PUBLIC_LOG_D32 " nextW=" PUBLIC_LOG_D32
            " nextH=" PUBLIC_LOG_D32, surfaceWidth_, surfaceHeight_, width, height);
    }
    surfaceWidth_ = width;
    surfaceHeight_ = height;
 
    MEDIA_LOG_I("ReportVideoSizeChange videoWidth: " PUBLIC_LOG_D32 " videoHeight: "
        PUBLIC_LOG_D32, surfaceWidth_, surfaceHeight_);
    std::pair<int32_t, int32_t> videoSize {surfaceWidth_, surfaceHeight_};
    eventReceiver_->OnEvent({"DecoderSurfaceFilter", EventType::EVENT_RESOLUTION_CHANGE, videoSize});
}

void DecoderSurfaceFilter::RegisterVideoFrameReadyCallback(std::shared_ptr<VideoFrameReadyCallback> &callback)
{
    std::unique_lock<std::mutex> draggingLock(draggingMutex_);
    isInSeekContinous_ = true;
    FALSE_RETURN(callback != nullptr);
    videoFrameReadyCallback_ = callback;
    FALSE_RETURN_NOLOG(postProcessor_ != nullptr);
    postProcessor_->StartSeekContinous();
}

void DecoderSurfaceFilter::DeregisterVideoFrameReadyCallback()
{
    std::unique_lock<std::mutex> draggingLock(draggingMutex_);
    isInSeekContinous_ = false;
    videoFrameReadyCallback_ = nullptr;
    FALSE_RETURN_NOLOG(postProcessor_ != nullptr);
    postProcessor_->StopSeekContinous();
}

Status DecoderSurfaceFilter::StartSeekContinous()
{
    isInSeekContinous_ = true;
    return Status::OK;
}

Status DecoderSurfaceFilter::StopSeekContinous()
{
    isInSeekContinous_ = false;
    return Status::OK;
}

int64_t DecoderSurfaceFilter::GetSystimeTimeNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
 
void DecoderSurfaceFilter::HandleFirstOutput()
{
    isRenderStarted_ = true;
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "ReportFirsrFrameEvent without eventReceiver_");
    eventReceiver_->OnEvent({"video_sink", EventType::EVENT_VIDEO_RENDERING_START, Status::OK});
}
 
void DecoderSurfaceFilter::HandleEosOutput(int index)
{
    int64_t curTimeNs = GetSystimeTimeNs();
    int64_t lastRenderTimeNs = lastRenderTimeNs_.load();
    if (lastRenderTimeNs == HST_TIME_NONE || curTimeNs >= lastRenderTimeNs || !eosTask_) {
        MEDIA_LOG_I("HandleEosOutput sync");
        DoReleaseOutputBuffer(index, false);
        ReportEosEvent();
        return;
    }
    std::weak_ptr<DecoderSurfaceFilter> weakPtr(shared_from_this());
    eosTask_->SubmitJobOnce([index, weakPtr] {
        MEDIA_LOG_I("HandleEosOutput async");
        auto strongPtr = weakPtr.lock();
        FALSE_RETURN(strongPtr != nullptr);
        int64_t lastRenderTimeNs = strongPtr->lastRenderTimeNs_.load();
        if (lastRenderTimeNs == HST_TIME_NONE ||
            lastRenderTimeNs - strongPtr->GetSystimeTimeNs() > TASK_DELAY_TOLERANCE) { // flush before release EOS
            return;
        }
        strongPtr->DoReleaseOutputBuffer(index, false);
        strongPtr->ReportEosEvent();
        }, (lastRenderTimeNs - curTimeNs) / MICROSECONDS_CONVERT_UNIT, false);
}
 
void DecoderSurfaceFilter::ReportEosEvent()
{
    MEDIA_LOG_I("ReportEOSEvent");
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "ReportEOSEvent without eventReceiver_");
    Event event {
        .srcFilter = "VideoSink",
        .type = EventType::EVENT_COMPLETE,
    };
    eventReceiver_->OnEvent(event);
}
 
void DecoderSurfaceFilter::RenderAtTimeDfx(int64_t renderTime, int64_t currentTime, int64_t lastRenderTimeNs)
{
    FALSE_RETURN_NOLOG(enableRenderAtTimeDfx_);
    renderTimeQueue_.push_back(renderTime);
 
    auto firstValidIt = std::lower_bound(renderTimeQueue_.begin(), renderTimeQueue_.end(), currentTime);
    renderTimeQueue_.erase(renderTimeQueue_.begin(), firstValidIt);
    MEDIA_LOG_D("RenderTimeQueue size = %{public}zu", renderTimeQueue_.size());
 
    int32_t count = 0;
    for (auto it = renderTimeQueue_.begin(); it != renderTimeQueue_.end(); ++it) {
        if (count < MAX_DEBUG_LOG) {
            logMessage += std::to_string(*it);
            logMessage += ", ";
            ++count;
        }
    }

    if (!logMessage.empty()) {
        MEDIA_LOG_D("buffer renderTime is %{public}s", logMessage.c_str());
        logMessage.clear();
    }
}

Status DecoderSurfaceFilter::SetSeiMessageCbStatus(bool status, const std::vector<int32_t> &payloadTypes)
{
    seiMessageCbStatus_ = status;
    MEDIA_LOG_I("seiMessageCbStatus_  = " PUBLIC_LOG_D32, seiMessageCbStatus_);
    FALSE_RETURN_V_MSG(
        inputBufferQueueProducer_ != nullptr, Status::ERROR_NO_MEMORY, "get producer failed");
    if (producerListener_ == nullptr) {
        producerListener_ =
            new SeiParserListener(codecMimeType_, inputBufferQueueProducer_, eventReceiver_, false);
        FALSE_RETURN_V_MSG(
            producerListener_ != nullptr, Status::ERROR_NO_MEMORY, "sei listener create failed");
    }
    producerListener_->SetSeiMessageCbStatus(status, payloadTypes);
    return Status::OK;
}

Status DecoderSurfaceFilter::InitPostProcessor()
{
    FALSE_RETURN_V_NOLOG(postProcessor_ != nullptr, Status::OK);
    postProcessor_->SetOutputSurface(videoSurface_);
    postProcessor_->SetEventReceiver(eventReceiver_);
    postProcessor_->SetVideoWindowSize(postProcessorTargetWidth_, postProcessorTargetHeight_);
    postProcessor_->SetPostProcessorOn(isPostProcessorOn_);
    postProcessor_->SetParameter(configFormat_);
    if (fdsanFd_) {
        postProcessor_->SetFd(fdsanFd_->Get());
        fdsanFd_->Reset();
    }
    auto ret = postProcessor_->Init();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Init postProcessor fail ret = %{public}d", ret);
        eventReceiver_->OnEvent({"decoderSurface", EventType::EVENT_ERROR, MSERR_UNSUPPORT_VID_SRC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    return Status::OK;
}

void DecoderSurfaceFilter::SetPostProcessorType(VideoPostProcessorType type)
{
    postProcessorType_ = type;
}

std::shared_ptr<BaseVideoPostProcessor> DecoderSurfaceFilter::CreatePostProcessor()
{
    MEDIA_LOG_D("CreatePostProcessor In");
    FALSE_RETURN_V_MSG(postProcessor_ == nullptr, nullptr, "Post processor is allready created!");
    FALSE_RETURN_V_MSG(postProcessorType_ != VideoPostProcessorType::NONE, nullptr, "post processor type is not set");
    postProcessor_ =
        VideoPostProcessorFactory::Instance().CreateVideoPostProcessor<BaseVideoPostProcessor>(postProcessorType_);
    if (postProcessor_ != nullptr) {
        decoderOutputSurface_ = postProcessor_->GetInputSurface();
        videoDecoder_->SetOutputSurface(decoderOutputSurface_);
        postProcessor_->SetOutputSurface(videoSurface_);
    }
    return postProcessor_;
}

Status DecoderSurfaceFilter::SetPostProcessorOn(bool isPostProcessorOn)
{
    isPostProcessorOn_ = isPostProcessorOn;
    FALSE_RETURN_V(isPostProcessorSupported_, Status::ERROR_UNSUPPORTED_FORMAT);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::OK);

    postProcessor_->SetPostProcessorOn(isPostProcessorOn);
    return Status::OK;
}

Status DecoderSurfaceFilter::SetVideoWindowSize(int32_t width, int32_t height)
{
    postProcessorTargetWidth_ = width;
    postProcessorTargetHeight_ = height;
    FALSE_RETURN_V(isPostProcessorSupported_, Status::ERROR_UNSUPPORTED_FORMAT);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::OK);

    return postProcessor_->SetVideoWindowSize(width, height);
}

Status DecoderSurfaceFilter::SetSpeed(float speed)
{
    FALSE_RETURN_V(videoDecoder_ != nullptr, Status::ERROR_INVALID_OPERATION);
    MEDIA_LOG_D("SetSpeed in");
    double frameRate = 0.0;
    configFormat_.GetDoubleValue(Tag::VIDEO_FRAME_RATE, frameRate);
    if (frameRate <= 0) {
        frameRate = DEFAULT_FRAME_RATE;
    }
    double frameRateWithSpeed = speed >= 1.0 ? frameRate * speed : frameRate;
    MEDIA_LOG_I("frameRate: %{public}f, speed: %{public}f, frameRateWithSpeed: %{public}f",
        frameRate, speed, frameRateWithSpeed);

    Format format;
    format.PutDoubleValue(Tag::VIDEO_FRAME_RATE, frameRateWithSpeed);
    videoDecoder_->SetParameter(format);
    FALSE_RETURN_V(postProcessor_ != nullptr, Status::OK);
    return postProcessor_->SetSpeed(speed);
}

Status DecoderSurfaceFilter::SetPostProcessorFd(int32_t postProcessorFd)
{
    FALSE_RETURN_V_MSG_E(postProcessorFd >= 0, Status::ERROR_INVALID_PARAMETER, "Invalid input fd.");
    int32_t dupFd = dup(postProcessorFd);
    FALSE_RETURN_V_MSG_E(dupFd >= 0, Status::ERROR_INVALID_PARAMETER, "Dup fd failed.");
    std::lock_guard<std::mutex> lock(fdMutex_);
    if (!fdsanFd_) {
        fdsanFd_ = std::make_unique<FdsanFd>(dupFd);
    } else {
        fdsanFd_->Reset(dupFd);
    }
    FALSE_RETURN_V(fdsanFd_ && (fdsanFd_->Get() >= 0), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}
 
Status DecoderSurfaceFilter::SetCameraPostprocessing(bool enable)
{
    MEDIA_LOG_I("SetCameraPostprocessing enter. %{public}d", enable);
    enableCameraPostprocessing_.store(enable);
    return Status::OK;
}
 
void DecoderSurfaceFilter::NotifyPause()
{
    MEDIA_LOG_D("NotifyPause enter.");
    FALSE_RETURN_NOLOG(postProcessor_ != nullptr);
    auto ret = postProcessor_->Pause();
    FALSE_RETURN_MSG(ret == Status::OK, "postProcessor pause error");
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
