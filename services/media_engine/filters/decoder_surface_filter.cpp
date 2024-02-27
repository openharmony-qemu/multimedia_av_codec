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
}

Status DecoderSurfaceFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure enter.");
    configureParameter_ = parameter;
    configFormat_.SetMeta(configureParameter_);
    videoDecoder_->Configure(configFormat_);
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCallback
        = std::make_shared<FilterMediaCodecCallback>(shared_from_this());
    videoDecoder_->SetCallback(mediaCodecCallback);
    return Status::OK;
}

Status DecoderSurfaceFilter::Prepare()
{
    MEDIA_LOG_I("Prepare enter.");
    if (onLinkedResultCallback_ != nullptr) {
        onLinkedResultCallback_->OnLinkedResult(videoDecoder_->GetInputBufferQueue(), meta_);
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::Start()
{
    MEDIA_LOG_I("Start enter.");
    Filter::Start();
    videoDecoder_->Start();
    return Status::OK;
}

Status DecoderSurfaceFilter::Pause()
{
    MEDIA_LOG_I("Pause enter.");
    Filter::Pause();
    videoDecoder_->Pause();
    videoSink_->ResetSyncInfo();
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

Status DecoderSurfaceFilter::Resume()
{
    MEDIA_LOG_I("Resume enter.");
    refreshTotalPauseTime_ = true;
    Filter::Resume();
    videoDecoder_->Resume();
    return Status::OK;
}

Status DecoderSurfaceFilter::Stop()
{
    MEDIA_LOG_I("Stop enter.");
    latestBufferTime_ = HST_TIME_NONE;
    latestPausedTime_ = HST_TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;

    timeval tv;
    gettimeofday(&tv, 0);
    stopTime_ = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec; // 1000000 means transfering from s to us.
    return Status::OK;
}

Status DecoderSurfaceFilter::Flush()
{
    MEDIA_LOG_I("Flush enter.");
    videoDecoder_->Flush();
    videoSink_->ResetSyncInfo();
    return Status::OK;
}

Status DecoderSurfaceFilter::Release()
{
    MEDIA_LOG_I("Release enter.");
    videoDecoder_->Release();
    return Status::OK;
}

void DecoderSurfaceFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter enter parameter is valid: %{public}i", parameter != nullptr);
    Format format;
    if (parameter->Find(Tag::VIDEO_SCALE_TYPE) != parameter->end()) {
        int32_t scaleType;
        parameter->Get<Tag::VIDEO_SCALE_TYPE>(scaleType);
        format.PutIntValue(Tag::VIDEO_SCALE_TYPE, scaleType);
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

    int32_t ret;
    // create secure decoder for drm.
    MEDIA_LOG_D("OnLinked enter the codecMimeType_ is %{public}s", codecMimeType_.c_str());
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
    if (ret != OHOS::MediaAVCodec::AVCodecServiceErrCode::AVCS_ERR_OK && eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({"decoderSurface", EventType::EVENT_ERROR, MSERR_UNSUPPORT_VID_DEC_TYPE});
    }

    Configure(meta);
    videoDecoder_->SetOutputSurface(videoSurface_);
    if (isDrmProtected_) {
        videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag_);
    }
    onLinkedResultCallback_ = callback;
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

void DecoderSurfaceFilter::DrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer)
{
    MEDIA_LOG_I("DrainOutputBuffer enter.");
    if (outputBuffer == nullptr) {
        MEDIA_LOG_E("DrainOutputBuffer error: outputBuffer is nullptr");
        return;
    }
    videoSink_->SetFirstPts(outputBuffer->pts_);
    videoDecoder_->ReleaseOutputBuffer(index, videoSink_, outputBuffer, true);
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