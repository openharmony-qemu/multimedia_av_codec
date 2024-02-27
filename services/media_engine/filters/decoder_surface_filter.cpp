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
#include "decoder_surface_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<DecoderSurfaceFilter> g_registerDecoderSurfaceFilter("builtin.player.videodecoder",
    FilterType::FILTERTYPE_VDEC, [](const std::string& name, const FilterType type) {
        return std::make_shared<DecoderSurfaceFilter>(name, FilterType::FILTERTYPE_VDEC);
    });

static const std::string VIDEO_INPUT_BUFFER_QUEUE_NAME = "VideoDecoderInputBufferQueue";

static const uint32_t MAX_SYNC_TIME = 1 * 1000 * 1000; // Sync over 1 second is considered invalid and drop it

class DecoderSurfaceFilterLinkCallback : public FilterLinkCallback {
public:
    explicit DecoderSurfaceFilterLinkCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter) {}

    ~DecoderSurfaceFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        MEDIA_LOG_I("OnLinkedResult enter.");
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
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
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
            decoderSurfaceFilter->ProcessInputBuffer();
        } else {
            MEDIA_LOG_I("invalid videoDecoder");
        }
    }

private:
    std::weak_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

DecoderSurfaceFilter::DecoderSurfaceFilter(const std::string& name, FilterType type): Filter(name, type)
{
    videoDecoder_ = std::make_shared<VideoDecoderAdapter>();
    videoSink_ = std::make_shared<VideoSink>();
    filterType_ = type;
}

DecoderSurfaceFilter::~DecoderSurfaceFilter()
{
    MEDIA_LOG_I("~DecoderSurfaceFilter() enter.");
    videoDecoder_->Release();
    MEDIA_LOG_I("~DecoderSurfaceFilter() exit.");
}

void DecoderSurfaceFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init enter.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    videoSink_->SetEventReceiver(eventReceiver_);
    FALSE_RETURN(videoDecoder_ != nullptr);
    videoDecoder_->SetEventReceiver(eventReceiver_);
    Filter::ActiveAsyncMode();
}

Status DecoderSurfaceFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure enter.");
    configureParameter_ = parameter;
    configFormat_.SetMeta(configureParameter_);
    Status ret = videoDecoder_->Configure(configFormat_);
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback
        = std::make_shared<FilterMediaCodecCallback>(shared_from_this());
    videoDecoder_->SetCallback(mediaCodecCallback);
    return ret;
}

Status DecoderSurfaceFilter::DoInit()
{
    Status ret;
    // create secure decoder for drm.
    MEDIA_LOG_I("DoInit enter the codecMimeType_ is %{public}s", codecMimeType_.c_str());
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
        return ret;
    }

    ret = Configure(meta_);
    FALSE_RETURN_V(ret == Status::OK, ret);
    videoDecoder_->SetOutputSurface(videoSurface_);
    if (isDrmProtected_) {
        videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare enter.");
    if (onLinkedResultCallback_ != nullptr) {
        if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
            MEDIA_LOG_W("InputBufferQueue already create");
            return Status::OK;
        }
        inputBufferQueue_ = AVBufferQueue::Create(0, MemoryType::UNKNOWN_MEMORY, VIDEO_INPUT_BUFFER_QUEUE_NAME, true);
        inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
        inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
        sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
        MEDIA_LOG_I("InputBufferQueue setlistener");
        inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
        videoDecoder_->SetInputBufferQueue(inputBufferQueueConsumer_);
        onLinkedResultCallback_->OnLinkedResult(inputBufferQueueProducer_, meta_);
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::DoStart()
{
    MEDIA_LOG_I("Start enter.");
    return videoDecoder_->Start();
}

Status DecoderSurfaceFilter::DoPause()
{
    MEDIA_LOG_I("Pause enter.");
    videoSink_->ResetSyncInfo();
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

Status DecoderSurfaceFilter::DoResume()
{
    MEDIA_LOG_I("Resume enter.");
    refreshTotalPauseTime_ = true;
    videoDecoder_->Start(); // codec already start
    return Status::OK;
}

Status DecoderSurfaceFilter::DoStop()
{
    MEDIA_LOG_I("Stop enter.");
    latestBufferTime_ = HST_TIME_NONE;
    latestPausedTime_ = HST_TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;

    timeval tv;
    gettimeofday(&tv, 0);
    stopTime_ = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec; // 1000000 means transfering from s to us.
    return videoDecoder_->Stop();
}

Status DecoderSurfaceFilter::DoFlush()
{
    MEDIA_LOG_I("Flush enter.");
    videoDecoder_->Flush();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        MEDIA_LOG_I("Flush enter.");
        outputBuffers_.clear();
    }
    videoSink_->ResetSyncInfo();
    return Status::OK;
}

Status DecoderSurfaceFilter::DoRelease()
{
    MEDIA_LOG_I("Release enter.");
    videoDecoder_->Release();
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
    MEDIA_LOG_I("SetParameter enter parameter is valid: %{public}i", parameter != nullptr);
    Format format;
    if (parameter->Find(Tag::VIDEO_SCALE_TYPE) != parameter->end()) {
        int32_t scaleType;
        parameter->Get<Tag::VIDEO_SCALE_TYPE>(scaleType);
        int32_t codecScalingMode = static_cast<int32_t>(ConvertMediaScaleType(static_cast<VideoScaleType>(scaleType)));
        format.PutIntValue(Tag::VIDEO_SCALE_TYPE, codecScalingMode);
        configFormat_.PutIntValue(Tag::VIDEO_SCALE_TYPE, scaleType);
    }
    // cannot set parameter when codec at [ CONFIGURED / INITIALIZED ] state
    auto ret = videoDecoder_->SetParameter(format);
    if (ret == MediaAVCodec::AVCS_ERR_INVALID_STATE) {
        MEDIA_LOG_W("SetParameter at invalid state");
        videoDecoder_->Reset();
        videoDecoder_->Configure(configFormat_);
        videoDecoder_->SetOutputSurface(videoSurface_);
        if (isDrmProtected_) {
            videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
        }
    }
    videoDecoder_->SetParameter(format);
}

void DecoderSurfaceFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter enter parameter is valid:  %{public}i", parameter != nullptr);
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
    MEDIA_LOG_I("OnLinked enter.");
    meta_ = meta;
    FALSE_RETURN_V_MSG(meta->GetData(Tag::MIME_TYPE, codecMimeType_),
        Status::ERROR_INVALID_PARAMETER, "get mime failed.");

    onLinkedResultCallback_ = callback;
    Filter::OnLinked(inType, meta, callback);
    return Status::OK;
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
    MEDIA_LOG_I("OnLinkedResult enter.");
}

void DecoderSurfaceFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
}

void DecoderSurfaceFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
}

Status DecoderSurfaceFilter::DoProcessOutputBuffer(int arg, bool dropped)
{
    std::pair<int, std::shared_ptr<AVBuffer>> task;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (outputBuffers_.empty()) {
            return Status::OK;
        }
        task = std::move(outputBuffers_.front());
        outputBuffers_.pop_front();
        if (!outputBuffers_.empty()) {
            std::pair<int, std::shared_ptr<AVBuffer>> nextTask = outputBuffers_.front();
            CalculateNextRender(nextTask.first, nextTask.second);
        }
    }
    videoDecoder_->ReleaseOutputBuffer(task.first, arg);
    return Status::OK;
}

Status DecoderSurfaceFilter::DoProcessInputBuffer(int arg, bool dropped)
{
    // input buffers will be detach in DoFlush, no need handle
    if (dropped) {
        return Status::OK;
    }
    videoDecoder_->AquireAvailableInputBuffer();
    return Status::OK;
}

int64_t DecoderSurfaceFilter::CalculateNextRender(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    int64_t waitTime = videoSink_->DoSyncWrite(outputBuffer);
    // Over 1 second is considered invalid and drop it
    if (waitTime >= MAX_SYNC_TIME) {
        waitTime = -1;
    }
    // waitTime < 0 will be dropped
    Filter::ProcessOutputBuffer(waitTime >= 0, waitTime);
    return waitTime;
}

void DecoderSurfaceFilter::DrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOG_I("DrainOutputBuffer enter. pts: " PUBLIC_LOG_D64"  outputSize:%{public}d",
        outputBuffer->pts_, outputBuffers_.size());
    videoSink_->SetFirstPts(outputBuffer->pts_);
    if (outputBuffers_.empty()) {
        CalculateNextRender(index, outputBuffer);
    }
    outputBuffers_.push_back(make_pair(index, outputBuffer));
}

Status DecoderSurfaceFilter::SetVideoSurface(sptr<Surface> videoSurface)
{
    if (!videoSurface) {
        MEDIA_LOG_W("videoSurface is null");
        return Status::ERROR_INVALID_PARAMETER;
    }
    videoSurface_ = videoSurface;
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
    MEDIA_LOG_I("SetDecryptConfig enter.");
    if (keySessionProxy == nullptr) {
        MEDIA_LOG_E("SetDecryptConfig keySessionProxy is nullptr.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    isDrmProtected_ = true;
    keySessionServiceProxy_ = keySessionProxy;
    svpFlag_ = svp;
    return Status::OK;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
