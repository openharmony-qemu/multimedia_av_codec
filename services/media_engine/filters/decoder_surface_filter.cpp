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
#include "video_decoder_adapter.h"
#include "osal/utils/util.h"
#include "parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "DecoderSurfaceFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
static const uint32_t LOCK_WAIT_TIME = 1000; // Lock wait for 1000ms.
static const int64_t PLAY_RANGE_DEFAULT_VALUE = -1;
static const int64_t MICROSECONDS_CONVERT_UNIT = 1000; // ms change to us

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
};

class FilterMediaCodecCallback : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit FilterMediaCodecCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter) {}

    ~FilterMediaCodecCallback() = default;

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
            decoderSurfaceFilter->DrainOutputBuffer(index, buffer);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
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
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->HandleInputBuffer();
        } else {
            MEDIA_LOG_I("invalid videoDecoder");
        }
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
}

DecoderSurfaceFilter::~DecoderSurfaceFilter()
{
    MEDIA_LOG_I("~DecoderSurfaceFilter()");
    if (!IS_FILTER_ASYNC && !isThreadExit_) {
        isThreadExit_ = true;
        condBufferAvailable_.notify_all();
        if (readThread_ != nullptr && readThread_->joinable()) {
            readThread_->join();
            readThread_ = nullptr;
        }
    }
    videoDecoder_->Release();
    MEDIA_LOG_I("~DecoderSurfaceFilter() exit.");
}

void DecoderSurfaceFilter::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOG_E("AVCodec error happened. ErrorType: %{public}d, errorCode: %{public}d",
        static_cast<int32_t>(errorType), errorCode);
    if (eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({"DecoderSurfaceFilter", EventType::EVENT_ERROR, MSERR_EXT_API9_IO});
    }
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
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback
        = std::make_shared<FilterMediaCodecCallback>(shared_from_this());
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
    }
    if (isDrmProtected_ && svpFlag_) {
        MEDIA_LOG_D("DecoderSurfaceFilter will create secure decoder for drm-protected videos");
        std::string baseName = GetCodecName(codecMimeType_);
        FALSE_RETURN_V_MSG(!baseName.empty(),
            Status::ERROR_INVALID_PARAMETER, "get name by mime failed.");
        std::string secureDecoderName = baseName + ".secure";
        MEDIA_LOG_D("DecoderSurfaceFilter will create secure decoder %{public}s", secureDecoderName.c_str());
        ret = videoDecoder_->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, false, secureDecoderName);
    } else {
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
    videoDecoder_->SetOutputSurface(videoSurface_);
    if (isDrmProtected_) {
        videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
    }
    videoSink_->SetParameter(meta_);
    return Status::OK;
}

Status DecoderSurfaceFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    if (onLinkedResultCallback_ != nullptr) {
        videoDecoder_->PrepareInputBufferQueue();
        sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
        sptr<Media::AVBufferQueueConsumer> inputBufferQueueConsumer = videoDecoder_->GetBufferQueueConsumer();
        inputBufferQueueConsumer->SetBufferAvailableListener(listener);
        onLinkedResultCallback_->OnLinkedResult(videoDecoder_->GetBufferQueueProducer(), meta_);
    }
    isRenderStarted_ = false;
    return Status::OK;
}

Status DecoderSurfaceFilter::DoPrepareFrame(bool renderFirstFrame)
{
    MEDIA_LOG_I("PrepareFrame");
    doPrepareFrame_ = true;
    renderFirstFrame_ = renderFirstFrame;
    Status ret = Status::OK;
    if (isPaused_.load()) {
        ret = DoResume();
    } else {
        ret = DoStart();
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("PrepareFrame decoder fail ret = %{public}d", ret);
        eventReceiver_->OnEvent({"decoderSurface", EventType::EVENT_ERROR, MSERR_VID_DEC_FAILED});
    }
    return ret;
}

Status DecoderSurfaceFilter::WaitPrepareFrame()
{
    MEDIA_LOG_D("WaitPrepareFrame");
    AutoLock lock(firstFrameMutex_);
    bool res = firstFrameCond_.WaitFor(lock, LOCK_WAIT_TIME, [this] {
         return !doPrepareFrame_;
    });
    MEDIA_LOG_I("PrepareFrame res= %{public}d.", res);
    doPrepareFrame_ = false;
    DoPause();
    return Status::OK;
}

Status DecoderSurfaceFilter::HandleInputBuffer()
{
    if (doPrepareFrame_) {
        MEDIA_LOG_I("doPrepareFrame");
        DoProcessInputBuffer(0, false);
    } else {
        ProcessInputBuffer();
    }
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
    return videoDecoder_->Start();
}

Status DecoderSurfaceFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    isPaused_ = true;
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_all();
    }
    videoSink_->ResetSyncInfo();
    latestPausedTime_ = latestBufferTime_;
    if (videoDecoder_ != nullptr) {
        videoDecoder_->ResetRenderTime();
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    refreshTotalPauseTime_ = true;
    isPaused_ = false;
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_all();
    }
    videoDecoder_->Start();
    return Status::OK;
}

Status DecoderSurfaceFilter::DoResumeDragging()
{
    MEDIA_LOG_I("DoResumeDragging enter.");
    refreshTotalPauseTime_ = true;
    isPaused_ = false;
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_all();
    }
    videoDecoder_->Start();
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
    playRangeStartTime_ = PLAY_RANGE_DEFAULT_VALUE;
    playRangeEndTime_ = PLAY_RANGE_DEFAULT_VALUE;

    timeval tv;
    gettimeofday(&tv, 0);
    stopTime_ = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec; // 1000000 means transfering from s to us.
    videoSink_->ResetSyncInfo();
    auto ret = videoDecoder_->Stop();
    if (!IS_FILTER_ASYNC && !isThreadExit_.load()) {
        isThreadExit_ = true;
        condBufferAvailable_.notify_all();
        if (readThread_ != nullptr && readThread_->joinable()) {
            readThread_->join();
            readThread_ = nullptr;
        }
    }
    return ret;
}

Status DecoderSurfaceFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    videoDecoder_->Flush();
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
    return Status::OK;
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
        int32_t codecScalingMode = static_cast<int32_t>(ConvertMediaScaleType(static_cast<VideoScaleType>(scaleType)));
        format.PutIntValue(Tag::VIDEO_SCALE_TYPE, codecScalingMode);
        configFormat_.PutIntValue(Tag::VIDEO_SCALE_TYPE, codecScalingMode);
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
            rate = 30.0; // 30.0 is the hisi default frame rate.
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
        videoDecoder_->SetOutputSurface(videoSurface_);
        if (isDrmProtected_) {
            videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
        }
    }
    videoDecoder_->SetParameter(format);
}

Status DecoderSurfaceFilter::GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration)
{
    if (videoDecoder_ == nullptr) {
        return Status::ERROR_INVALID_OPERATION;
    }
    return videoDecoder_->GetLagInfo(lagTimes, maxLagDuration, avgLagDuration);
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
    if (codeclist == nullptr) {
        MEDIA_LOG_E("GetCodecName failed due to codeclist nullptr.");
        return codecName;
    }
    MediaAVCodec::Format format;
    format.PutStringValue("codec_mime", mimeType);
    codecName = codeclist->FindDecoder(format);
    return codecName;
}

Status DecoderSurfaceFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    meta_ = meta;
    FALSE_RETURN_V_MSG(meta->GetData(Tag::MIME_TYPE, codecMimeType_),
        Status::ERROR_INVALID_PARAMETER, "get mime failed.");

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
    bool acquireRes = AcquireNextRenderBuffer(byIdx, index, outputBuffer);
    FALSE_RETURN_V(acquireRes, Status::OK);
    ReleaseOutputBuffer(index, recvArg, outputBuffer, renderTime);
    return Status::OK;
}

bool DecoderSurfaceFilter::AcquireNextRenderBuffer(bool byIdx, uint32_t &index, std::shared_ptr<AVBuffer> &outBuffer)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!byIdx) {
        FALSE_RETURN_V(!outputBuffers_.empty(), false);
        std::pair<int, std::shared_ptr<AVBuffer>> task = std::move(outputBuffers_.front());
        outputBuffers_.pop_front();
        if (!outputBuffers_.empty()) {
            std::pair<int, std::shared_ptr<AVBuffer>> nextTask = outputBuffers_.front();
            RenderNextOutput(nextTask.first, nextTask.second);
        }
        index = static_cast<int32_t>(task.first);
        outBuffer = task.second;
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
    if (render && !isRenderStarted_.load()) {
        isRenderStarted_ = true;
        eventReceiver_->OnEvent({"video_sink", EventType::EVENT_VIDEO_RENDERING_START, Status::OK});
    }
    if ((playRangeEndTime_ != PLAY_RANGE_DEFAULT_VALUE) &&
        (outBuffer->pts_ > playRangeEndTime_ * MICROSECONDS_CONVERT_UNIT)) {
        MEDIA_LOG_I("ReleaseBuffer for eos, SetPlayRange start: " PUBLIC_LOG_D64 ", end: " PUBLIC_LOG_D32,
                    playRangeStartTime_, playRangeEndTime_);
        if (eventReceiver_ != nullptr) {
            Event event {
                .srcFilter = "VideoSink",
                .type = EventType::EVENT_COMPLETE,
            };
            eventReceiver_->OnEvent(event);
        }
        return Status::OK;
    }
    if (renderTime > 0L && render) {
        videoDecoder_->RenderOutputBufferAtTime(index, renderTime);
    } else if (outBuffer->pts_ < 0) {
        MEDIA_LOG_W("Avoid render video frame with pts=%{public}" PUBLIC_LOG_D64, outBuffer->pts_);
        videoDecoder_->ReleaseOutputBuffer(index, false);
    } else {
        videoDecoder_->ReleaseOutputBuffer(index, render);
    }
    if (!isInSeekContinous_) {
        videoSink_->SetLastPts(outBuffer->pts_);
    }
    if ((outBuffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)) && !isInSeekContinous_) {
        ResetSeekInfo();
        MEDIA_LOG_I("ReleaseBuffer for eos, index: %{public}u,  bufferid: %{public}" PRIu64
                ", pts: %{public}" PRIu64", flag: %{public}u", index, outBuffer->GetUniqueId(),
                outBuffer->pts_, outBuffer->flag_);
        if (eventReceiver_ != nullptr) {
            Event event {
                .srcFilter = "VideoSink",
                .type = EventType::EVENT_COMPLETE,
            };
            eventReceiver_->OnEvent(event);
        }
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoProcessInputBuffer(int recvArg, bool dropFrame)
{
    videoDecoder_->AquireAvailableInputBuffer();
    return Status::OK;
}

int64_t DecoderSurfaceFilter::CalculateNextRender(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    int64_t waitTime = -1;
    if (isSeek_) {
        if (outputBuffer->pts_ >= seekTimeUs_) {
            MEDIA_LOG_D("DrainOutputBuffer is seeking and render. pts: " PUBLIC_LOG_D64, outputBuffer->pts_);
            // In order to be compatible with live stream, audio and video synchronization uses the relative
            // value of pts. The first frame pts must be the first frame displayed, not the first frame sent.
            videoSink_->SetFirstPts(outputBuffer->pts_);
            waitTime = videoSink_->DoSyncWrite(outputBuffer);
            isSeek_ = false;
        } else {
            MEDIA_LOG_D("DrainOutputBuffer is seeking and not render. pts: " PUBLIC_LOG_D64, outputBuffer->pts_);
        }
    } else {
        MEDIA_LOG_D("DrainOutputBuffer not seeking and render. pts: " PUBLIC_LOG_D64, outputBuffer->pts_);
        videoSink_->SetFirstPts(outputBuffer->pts_);
        waitTime = videoSink_->DoSyncWrite(outputBuffer);
    }
    return waitTime;
}

// async filter should call this function
void DecoderSurfaceFilter::RenderNextOutput(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (isInSeekContinous_) {
        Filter::ProcessOutputBuffer(false, 0);
    }
    int64_t waitTime = CalculateNextRender(index, outputBuffer);
    MEDIA_LOG_D("RenderNextOutput pts: " PUBLIC_LOG_D64"  waitTime: " PUBLIC_LOG_D64,
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
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOG_D("DrainOutputBuffer pts: " PUBLIC_LOG_D64"  outputSize:%{public}d",
        outputBuffer->pts_, outputBuffers_.size());
    if (isInSeekContinous_) {
        if (videoFrameReadyCallback_ != nullptr) {
            MEDIA_LOG_D("[drag_debug]DrainOutputBuffer2 dts: " PUBLIC_LOG_D64 ", pts: " PUBLIC_LOG_D64
                        " bufferIdx: " PUBLIC_LOG_D32,
                        outputBuffer->dts_, outputBuffer->pts_, index);
            videoFrameReadyCallback_->ConsumeVideoFrame(outputBuffer, index);
        }
        outputBufferMap_.insert(std::make_pair(index, outputBuffer));
        return;
    }
    if (doPrepareFrame_.load()) {
        if (renderFirstFrame_ && !(outputBuffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS))) {
            videoDecoder_->ReleaseOutputBuffer(index, true);
        } else {
            outputBuffers_.push_back(make_pair(index, outputBuffer));
            if (IS_FILTER_ASYNC) {
                Filter::ProcessOutputBuffer(1, false); // 1 indicate to render
            }
        }
        AutoLock autolock(firstFrameMutex_);
        doPrepareFrame_ = false;
        firstFrameCond_.NotifyAll();
        return;
    }
    if (IS_FILTER_ASYNC && outputBuffers_.empty()) {
        RenderNextOutput(index, outputBuffer);
    }
    outputBuffers_.push_back(make_pair(index, outputBuffer));
    if (!IS_FILTER_ASYNC) {
        condBufferAvailable_.notify_one();
    }
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

Status DecoderSurfaceFilter::SetVideoSurface(sptr<Surface> videoSurface)
{
    if (!videoSurface) {
        MEDIA_LOG_W("videoSurface is null");
        return Status::ERROR_INVALID_PARAMETER;
    }
    videoSurface_ = videoSurface;
    if (videoDecoder_ != nullptr) {
        MEDIA_LOG_I("videoDecoder_ SetOutputSurface in");
        int32_t res = videoDecoder_->SetOutputSurface(videoSurface_);
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
    FALSE_RETURN(videoDecoder_ != nullptr);
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
    keySessionServiceProxy_ = keySessionProxy;
    svpFlag_ = svp;
    return Status::OK;
}

void DecoderSurfaceFilter::SetSeekTime(int64_t seekTimeUs)
{
    MEDIA_LOG_I("SetSeekTime");
    isSeek_ = true;
    seekTimeUs_ = seekTimeUs;
}

void DecoderSurfaceFilter::ResetSeekInfo()
{
    MEDIA_LOG_I("ResetSeekInfo");
    isSeek_ = false;
    seekTimeUs_ = 0;
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

void DecoderSurfaceFilter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("DecoderSurfaceFilter::OnDumpInfo called.");
    if (videoDecoder_ != nullptr) {
        videoDecoder_->OnDumpInfo(fd);
    }
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
    isInSeekContinous_ = true;
    if (callback != nullptr) {
        videoFrameReadyCallback_ = callback;
    }
}

void DecoderSurfaceFilter::DeregisterVideoFrameReadyCallback()
{
    isInSeekContinous_ = false;
    videoFrameReadyCallback_ = nullptr;
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
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
