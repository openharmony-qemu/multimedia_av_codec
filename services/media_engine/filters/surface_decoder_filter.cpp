/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "surface_decoder_filter.h"
#include "surface_encoder_filter.h"
#include "filter/filter_factory.h"
#include "surface_decoder_adapter.h"
#include "meta/format.h"
#include "common/media_core.h"
#include "surface/native_buffer.h"
#include "media_description.h"
#include "av_common.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "SurfaceDecoderFilter" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<SurfaceDecoderFilter> g_registerSurfaceDecoderFilter("builtin.player.surfacedecoder",
    FilterType::FILTERTYPE_VIDEODEC, [](const std::string& name, const FilterType type) {
        return std::make_shared<SurfaceDecoderFilter>(name, FilterType::FILTERTYPE_VIDEODEC);
    });

class SurfaceDecoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit SurfaceDecoderFilterLinkCallback(std::shared_ptr<SurfaceDecoderFilter> surfaceDecoderFilter)
        : surfaceDecoderFilter_(std::move(surfaceDecoderFilter))
    {
    }

    ~SurfaceDecoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) {
            surfaceDecoderFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) {
            surfaceDecoderFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) {
            surfaceDecoderFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderFilter");
        }
    }
private:
    std::weak_ptr<SurfaceDecoderFilter> surfaceDecoderFilter_;
};

class SurfaceDecoderAdapterCallback : public DecoderAdapterCallback {
public:
    explicit SurfaceDecoderAdapterCallback(std::shared_ptr<SurfaceDecoderFilter> surfaceDecoderFilter)
        : surfaceDecoderFilter_(std::move(surfaceDecoderFilter))
    {
    }

    void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode) override
    {
    }

    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) override
    {
    }

    void OnBufferEos(int64_t pts) override
    {
        if (auto surfaceDecoderFilter = surfaceDecoderFilter_.lock()) {
            surfaceDecoderFilter->NotifyNextFilterEos(pts);
        } else {
            MEDIA_LOG_I("invalid surfaceDecoderFilter");
        }
    }
private:
    std::weak_ptr<SurfaceDecoderFilter> surfaceDecoderFilter_;
};

SurfaceDecoderFilter::SurfaceDecoderFilter(const std::string& name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("surface decoder filter create");
}

SurfaceDecoderFilter::~SurfaceDecoderFilter()
{
    MEDIA_LOG_I("surface decoder filter destroy");
}

void SurfaceDecoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status SurfaceDecoderFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    if (mediaCodec_ == nullptr) {
        MEDIA_LOG_E("mediaCodec is null");
        return Status::ERROR_UNKNOWN;
    }
    configureParameter_ = parameter;
    configFormat_.SetMeta(configureParameter_);
    bool isHdr = false;
    configureParameter_->GetData(Tag::VIDEO_IS_HDR_VIVID, isHdr);
    if (isHdr) {
        MEDIA_LOG_D("isHdr true,set video_decoder_output_colorspace, pixel_format");
        configFormat_.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            static_cast<int8_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT));
        configFormat_.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
            static_cast<int8_t>(MediaAVCodec::VideoPixelFormat::NV12));
    } else {
        MEDIA_LOG_D("isHdr false");
    }
    configFormat_.PutIntValue(Tag::VIDEO_FRAME_RATE_ADAPTIVE_MODE, true);
    Status ret = mediaCodec_->Configure(configFormat_);
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Configure fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
    return ret;
}

Status SurfaceDecoderFilter::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetOutputSurface");
    if (mediaCodec_ == nullptr) {
        MEDIA_LOG_E("mediaCodec is null");
        return Status::ERROR_UNKNOWN;
    }
    outputSurface_ = surface;
    Status ret = mediaCodec_->SetOutputSurface(outputSurface_);
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec SetOutputSurface fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
    return ret;
}

Status SurfaceDecoderFilter::NotifyNextFilterEos(int64_t pts)
{
    MEDIA_LOG_I("NotifyNextFilterEos");
    for (auto iter : nextFiltersMap_) {
        for (auto filter : iter.second) {
            std::shared_ptr<Meta> eosMeta = std::make_shared<Meta>();
            eosMeta->Set<Tag::MEDIA_END_OF_STREAM>(true);
            MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64, pts);
            eosMeta->Set<Tag::USER_FRAME_PTS>(pts);
            filter->SetParameter(eosMeta);
        }
    }
    return Status::OK;
}

Status SurfaceDecoderFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    if (filterCallback_ == nullptr) {
        MEDIA_LOG_E("filterCallback is null");
        return Status::ERROR_UNKNOWN;
    }
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_RAW_VIDEO);
    return Status::OK;
}

Status SurfaceDecoderFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    if (mediaCodec_ == nullptr) {
        MEDIA_LOG_E("mediaCodec is null");
        return Status::ERROR_UNKNOWN;
    }
    Status ret = mediaCodec_->Start();
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Start fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
    return ret;
}

Status SurfaceDecoderFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    if (mediaCodec_ == nullptr) {
        MEDIA_LOG_E("mediaCodec is null");
        return Status::ERROR_UNKNOWN;
    }
    Status ret = mediaCodec_->Pause();
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Pause fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
    return ret;
}

Status SurfaceDecoderFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    if (mediaCodec_ == nullptr) {
        MEDIA_LOG_E("mediaCodec is null");
        return Status::ERROR_UNKNOWN;
    }
    Status ret = mediaCodec_->Resume();
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Resume fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
    return ret;
}

Status SurfaceDecoderFilter::DoStop()
{
    MEDIA_LOG_I("Stop enter");
    if (mediaCodec_ == nullptr) {
        return Status::OK;
    }
    Status ret = mediaCodec_->Stop();
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Stop fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
        return ret;
    }
    MEDIA_LOG_I("Stop done");
    return Status::OK;
}

Status SurfaceDecoderFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    return Status::OK;
}

Status SurfaceDecoderFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    return Status::OK;
}

void SurfaceDecoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    if (mediaCodec_ == nullptr) {
        return;
    }
    Format format;
    format.SetMeta(parameter);
    auto ret = mediaCodec_->SetParameter(format);
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec SetParameter fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_decoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
}

void SurfaceDecoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status SurfaceDecoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<SurfaceDecoderFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    return Status::OK;
}

Status SurfaceDecoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status SurfaceDecoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType SurfaceDecoderFilter::GetFilterType()
{
    return filterType_;
}

Status SurfaceDecoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    FALSE_RETURN_V_MSG(meta->GetData(Tag::MIME_TYPE, codecMimeType_),
        Status::ERROR_INVALID_PARAMETER, "get mime failed.");
    MEDIA_LOG_I("OnLinked enter the codecMimeType_ is %{public}s", codecMimeType_.c_str());
    mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    Status ret = mediaCodec_->Init(codecMimeType_);
    if (ret == Status::OK) {
        std::shared_ptr<DecoderAdapterCallback> decoderSurfaceCallback =
            std::make_shared<SurfaceDecoderAdapterCallback>(shared_from_this());
        mediaCodec_->SetDecoderAdapterCallback(decoderSurfaceCallback);
    } else {
        MEDIA_LOG_E("Init mediaCodec fail");
        if (eventReceiver_ != nullptr) {
            eventReceiver_->OnEvent({"surface_encoder_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
    }
    meta_ = meta;
    ret = Configure(meta);
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Configure fail");
    }
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status SurfaceDecoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status SurfaceDecoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void SurfaceDecoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult");
    (void) meta;
    if (onLinkedResultCallback_) {
        onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta_);
    }
}

void SurfaceDecoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
}

void SurfaceDecoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
