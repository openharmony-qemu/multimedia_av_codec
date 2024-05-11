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

#include <iostream>
#include <string>
#include "surface_encoder_filter.h"
#include "filter/filter_factory.h"
#include "surface_encoder_adapter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<SurfaceEncoderFilter> g_registerSurfaceEncoderFilter("builtin.recorder.videoencoder",
    FilterType::FILTERTYPE_VENC,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<SurfaceEncoderFilter>(name, FilterType::FILTERTYPE_VENC);
    });

class SurfaceEncoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit SurfaceEncoderFilterLinkCallback(std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter)
        : surfaceEncoderFilter_(std::move(surfaceEncoderFilter))
    {
    }

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto surfaceEncoderFilter = surfaceEncoderFilter_.lock()) {
            surfaceEncoderFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto surfaceEncoderFilter = surfaceEncoderFilter_.lock()) {
            surfaceEncoderFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto surfaceEncoderFilter = surfaceEncoderFilter_.lock()) {
            surfaceEncoderFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderFilter");
        }
    }
private:
    std::weak_ptr<SurfaceEncoderFilter> surfaceEncoderFilter_;
};

class SurfaceEncoderAdapterCallback : public EncoderAdapterCallback {
public:
    SurfaceEncoderAdapterCallback()
    {
    }

    void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode)
    {
    }

    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format)
    {
    }
};

SurfaceEncoderFilter::SurfaceEncoderFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("encoder filter create");
}

SurfaceEncoderFilter::~SurfaceEncoderFilter()
{
    MEDIA_LOG_I("encoder filter destroy");
}

Status SurfaceEncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    std::string codecMimeType;
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType), Status::ERROR_INVALID_PARAMETER);
    if (strcmp(codecMimeType.c_str(), codecMimeType_.c_str()) != 0) {
        isUpdateCodecNeeded_ = true;
    }
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void SurfaceEncoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    if (!mediaCodec_ || isUpdateCodecNeeded_) {
        if (mediaCodec_) {
            mediaCodec_->Release();
        }
        mediaCodec_ = std::make_shared<SurfaceEncoderAdapter>();
        Status ret = mediaCodec_->Init(codecMimeType_, true);
        if (ret == Status::OK) {
            std::shared_ptr<EncoderAdapterCallback> encoderAdapterCallback =
                std::make_shared<SurfaceEncoderAdapterCallback>();
            mediaCodec_->SetEncoderAdapterCallback(encoderAdapterCallback);
            mediaCodec_->SetCallingInfo(appUid_, appPid_, bundleName_, instanceId_);
        } else {
            MEDIA_LOG_I("Init mediaCodec fail");
            eventReceiver_->OnEvent({"surface_encoder_filter", EventType::EVENT_ERROR, Status::ERROR_UNKNOWN});
        }
        surface_ = nullptr;
        isUpdateCodecNeeded_ = false;
    }
}

Status SurfaceEncoderFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    if (mediaCodec_ == nullptr) {
        return Status::ERROR_UNKNOWN;
    }
    configureParameter_ = parameter;
    return mediaCodec_->Configure(parameter);
}

Status SurfaceEncoderFilter::SetInputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetInputSurface");
    mediaCodec_->SetInputSurface(surface);
    return Status::OK;
}

sptr<Surface> SurfaceEncoderFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    if (surface_) {
        return surface_;
    }
    surface_ = mediaCodec_->GetInputSurface();
    return surface_;
}

Status SurfaceEncoderFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_ENCODED_VIDEO);
    return Status::OK;
}

Status SurfaceEncoderFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    if (mediaCodec_ == nullptr) {
        return Status::ERROR_UNKNOWN;
    }
    return mediaCodec_->Start();
}

Status SurfaceEncoderFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    if (mediaCodec_ == nullptr) {
        return Status::ERROR_UNKNOWN;
    }
    return mediaCodec_->Pause();
}

Status SurfaceEncoderFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    if (mediaCodec_ == nullptr) {
        return Status::ERROR_UNKNOWN;
    }
    return mediaCodec_->Resume();
}

Status SurfaceEncoderFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    if (mediaCodec_ == nullptr) {
        return Status::OK;
    }
    mediaCodec_->Stop();
    return Status::OK;
}

Status SurfaceEncoderFilter::Reset()
{
    MEDIA_LOG_I("Reset");
    if (mediaCodec_ == nullptr) {
        return Status::OK;
    }
    mediaCodec_->Reset();
    return Status::OK;
}

Status SurfaceEncoderFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    return mediaCodec_->Flush();
}

Status SurfaceEncoderFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    if (mediaCodec_ == nullptr) {
        return Status::OK;
    }
    return mediaCodec_->Reset();
}

Status SurfaceEncoderFilter::NotifyEos()
{
    MEDIA_LOG_I("NotifyEos");
    return mediaCodec_->NotifyEos();
}

void SurfaceEncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    if (mediaCodec_ == nullptr) {
        return;
    }
    mediaCodec_->SetParameter(parameter);
}

void SurfaceEncoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status SurfaceEncoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<SurfaceEncoderFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    return Status::OK;
}

Status SurfaceEncoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status SurfaceEncoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType SurfaceEncoderFilter::GetFilterType()
{
    return filterType_;
}

Status SurfaceEncoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    return Status::OK;
}

Status SurfaceEncoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status SurfaceEncoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void SurfaceEncoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult");
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
}

void SurfaceEncoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
}

void SurfaceEncoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
}

void SurfaceEncoderFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
    if (mediaCodec_) {
        mediaCodec_->SetCallingInfo(appUid, appPid, bundleName, instanceId);
    }
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS