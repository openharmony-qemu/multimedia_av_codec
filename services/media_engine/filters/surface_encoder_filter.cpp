/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "surface_encoder_filter.h"

#include <iostream>
#include <string>
#include "common/log.h"
#include "common/media_core.h"
#include "filter/filter_factory.h"
#include "surface_encoder_adapter.h"
#include "muxer_filter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_RECORDER, "SurfaceEncoderFilter" };
}

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
    explicit SurfaceEncoderAdapterCallback(std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter)
        : surfaceEncoderFilter_(std::move(surfaceEncoderFilter))
    {
    }

    void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode)
    {
        if (auto surfaceEncoderFilter = surfaceEncoderFilter_.lock()) {
            surfaceEncoderFilter->OnError(type, errorCode);
        } else {
            MEDIA_LOG_D("invalid surfaceEncoderFilter");
        }
    }

    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format)
    {
    }

private:
    std::weak_ptr<SurfaceEncoderFilter> surfaceEncoderFilter_;
};

class SurfaceEncoderAdapterKeyFramePtsCallback : public EncoderAdapterKeyFramePtsCallback {
public:
    explicit SurfaceEncoderAdapterKeyFramePtsCallback(std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter)
        : surfaceEncoderFilter_(std::move(surfaceEncoderFilter))
    {
    }
    
    void OnReportKeyFramePts(std::string KeyFramePts) override
    {
        if (auto surfaceEncoderFilter = surfaceEncoderFilter_.lock()) {
            MEDIA_LOG_D("SurfaceEncoderAdapterKeyFramePtsCallback OnReportKeyFramePts start");
            surfaceEncoderFilter->OnReportKeyFramePts(KeyFramePts);
            MEDIA_LOG_D("SurfaceEncoderAdapterKeyFramePtsCallback OnReportKeyFramePts end");
        } else {
            MEDIA_LOG_I("invalid surfaceEncoderFilter");
        }
    }

    void OnReportFirstFramePts(int64_t firstFramePts) override
    {
        if (auto surfaceEncoderFilter = surfaceEncoderFilter_.lock()) {
            MEDIA_LOG_D("SurfaceEncoderAdapterKeyFramePtsCallback OnReportFirstFramePts start");
            surfaceEncoderFilter->OnReportFirstFramePts(firstFramePts);
            MEDIA_LOG_D("SurfaceEncoderAdapterKeyFramePtsCallback OnReportFirstFramePts end");
        } else {
            MEDIA_LOG_W("invalid surfaceEncoderFilter");
        }
    }

private:
    std::weak_ptr<SurfaceEncoderFilter> surfaceEncoderFilter_;
};

SurfaceEncoderFilter::SurfaceEncoderFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("encoder filter create");
}

SurfaceEncoderFilter::~SurfaceEncoderFilter()
{
    MEDIA_LOG_I("encoder filter destroy");
}

void SurfaceEncoderFilter::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOG_E("AVCodec error happened. ErrorType: %{public}d, errorCode: %{public}d",
        static_cast<int32_t>(errorType), errorCode);
    if (eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({"surface_encoder_filter", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_VID_ENC_FAILED)});
    }
}

Status SurfaceEncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    FALSE_RETURN_V(format != nullptr, Status::ERROR_INVALID_PARAMETER);
    MEDIA_LOG_I("SetCodecFormat");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    std::string codecMimeType;
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType), Status::ERROR_INVALID_PARAMETER);
    if (codecMimeType != codecMimeType_) {
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
    Status ret = Status::OK;
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        if (!mediaCodec_ || isUpdateCodecNeeded_.load()) {
            if (mediaCodec_) {
                mediaCodec_->Release();
            }
            mediaCodec_ = std::make_shared<SurfaceEncoderAdapter>();
            mediaCodec_->SetCallingInfo(appUid_, appPid_, bundleName_, instanceId_);
            ret = mediaCodec_->Init(codecMimeType_, true);
            if (ret == Status::OK) {
                std::shared_ptr<EncoderAdapterCallback> encoderAdapterCallback =
                    std::make_shared<SurfaceEncoderAdapterCallback>(shared_from_this());
                mediaCodec_->SetEncoderAdapterCallback(encoderAdapterCallback);
                std::shared_ptr<EncoderAdapterKeyFramePtsCallback> encoderAdapterKeyFramePtsCallback =
                    std::make_shared<SurfaceEncoderAdapterKeyFramePtsCallback>(shared_from_this());
                mediaCodec_->SetEncoderAdapterKeyFramePtsCallback(encoderAdapterKeyFramePtsCallback);
            }
            surface_ = nullptr;
            isUpdateCodecNeeded_ = false;
        }
    }
    if (ret != Status::OK && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("Init mediaCodec fail");
        eventReceiver_->OnEvent({"surface_encoder_filter", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_UNKNOWN)});
    }
}

Status SurfaceEncoderFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    if (parameter == nullptr) {
        MEDIA_LOG_E("Configure parameter is nullptr");
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status ret = Status::ERROR_UNKNOWN;
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        if (mediaCodec_ == nullptr) {
            return Status::ERROR_UNKNOWN;
        }
        ret = mediaCodec_->Configure(parameter);
    }
    configureParameter_ = parameter;
    if (ret == Status::ERROR_VID_ENC_RESOLUTION_UNSUPPORTED && isTranscoderMode_.load() && eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({"surface_encoder_filter", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_TARGET_RESOLUTION_OUT_OF_RANGE)});
    }
    return ret;
}

Status SurfaceEncoderFilter::SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer)
{
    MEDIA_LOG_I("SetWatermark");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->SetWatermark(waterMarkBuffer);
}

Status SurfaceEncoderFilter::SetVideoEnableBFrame(bool &enableBFrame)
{
    MEDIA_LOG_I("SetVideoEnableBFrame");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->SetVideoEnableBFrame(enableBFrame);
}

Status SurfaceEncoderFilter::SetStopTime()
{
    MEDIA_LOG_I("SetStopTime");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->SetStopTime();
}

Status SurfaceEncoderFilter::SetInputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetInputSurface");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    mediaCodec_->SetInputSurface(surface);
    return Status::OK;
}

Status SurfaceEncoderFilter::SetTransCoderMode()
{
    MEDIA_LOG_I("SetTransCoderMode");
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
        mediaCodec_->SetTransCoderMode();
    }
    isTranscoderMode_.store(true);
    return Status::OK;
}

Status SurfaceEncoderFilter::SetTimeRange(int64_t startTimeMs, int64_t endTimeMs)
{
    MEDIA_LOG_I("SetTimeRange, startTimeMs: " PUBLIC_LOG_D64 ", endTimeMs: " PUBLIC_LOG_D64, startTimeMs, endTimeMs);
    startTimeMs_ = startTimeMs;
    return Status::OK;
}

void SurfaceEncoderFilter::SetClipStartRelativeUs(int64_t clipStartRelativeUs)
{
    MEDIA_LOG_I("SetClipStartRelativeUs: " PUBLIC_LOG_D64, clipStartRelativeUs);
    clipStartRelativeUs_ = clipStartRelativeUs;
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    if (mediaCodec_ != nullptr) {
        mediaCodec_->SetClipStartTime(clipStartRelativeUs);
    }
}

Status SurfaceEncoderFilter::SetWatermarkMode()
{
    MEDIA_LOG_I("SetWatermarkMode");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    mediaCodec_->SetWatermarkMode();
    return Status::OK;
}

Status SurfaceEncoderFilter::SetStabilizationMode(bool enableStabilization)
{
    MEDIA_LOG_I("SetStabilizationMode");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    mediaCodec_->SetStabilizationMode(enableStabilization);
    return Status::OK;
}

void SurfaceEncoderFilter::SetTransCoderCapabilityCheckInfo(int32_t width, int32_t height)
{
    MEDIA_LOG_I("SetTransCoderCapabilityCheckInfo width: %{public}d, height: %{public}d", width, height);
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    if (mediaCodec_ != nullptr) {
        mediaCodec_->SetTransCoderCapabilityCheckInfo(width, height);
    }
}

sptr<Surface> SurfaceEncoderFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    if (surface_ == nullptr && mediaCodec_ != nullptr) {
        surface_ = mediaCodec_->GetInputSurface();
    }
    return surface_;
}

Status SurfaceEncoderFilter::DoPrepare()
{
    FALSE_RETURN_V(filterCallback_ != nullptr, Status::ERROR_NULL_POINTER);
    MEDIA_LOG_I("Prepare");
    bool isTranscoderMode = false;
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        isTranscoderMode = isTranscoderMode_.load();
    }
    if (isTranscoderMode) {
        MEDIA_LOG_I("TranscoderMode");
        return filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
            StreamType::STREAMTYPE_ENCODED_VIDEO);
    }
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_ENCODED_VIDEO);
    return Status::OK;
}

Status SurfaceEncoderFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->Start();
}

Status SurfaceEncoderFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->Pause();
}

Status SurfaceEncoderFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->Resume();
}

Status SurfaceEncoderFilter::DoStop()
{
    MEDIA_LOG_I("Stop enter");
    std::shared_ptr<SurfaceEncoderAdapter> mediaCodec;
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        mediaCodec = mediaCodec_;
    }
    if (mediaCodec == nullptr) {
        return Status::OK;
    }
    Status ret = mediaCodec->Stop();
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec DoStop failed, ret=%{public}d", static_cast<int32_t>(ret));
    }
    return Status::OK;
}

Status SurfaceEncoderFilter::Reset()
{
    MEDIA_LOG_I("Reset");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    if (mediaCodec_ == nullptr) {
        return Status::OK;
    }
    Status ret = mediaCodec_->Reset();
    if (ret != Status::OK) {
        MEDIA_LOG_E("mediaCodec Reset failed, ret=%{public}d", static_cast<int32_t>(ret));
    }
    return Status::OK;
}

Status SurfaceEncoderFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->Flush();
}

Status SurfaceEncoderFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    if (mediaCodec_ == nullptr) {
        return Status::OK;
    }
    return mediaCodec_->Reset();
}

Status SurfaceEncoderFilter::NotifyEos(int64_t pts)
{
    MEDIA_LOG_I("NotifyEos");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    return NotifyEosLocked(pts);
}

Status SurfaceEncoderFilter::NotifyEosLocked(int64_t pts)
{
    FALSE_RETURN_V_MSG(mediaCodec_ != nullptr, Status::ERROR_UNKNOWN, "mediaCodec_ is nullptr.");
    return mediaCodec_->NotifyEos(pts);
}

void SurfaceEncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    FALSE_RETURN_MSG(parameter != nullptr, "SetParameter parameter is null");
    if (mediaCodec_ == nullptr) {
        return;
    }
    bool isEos = false;
    int64_t eosPts = UINT32_MAX;
    if (parameter->Find(Tag::MEDIA_END_OF_STREAM) != parameter->end() &&
        parameter->Get<Tag::MEDIA_END_OF_STREAM>(isEos) &&
        parameter->Get<Tag::USER_FRAME_PTS>(eosPts)) {
        if (isEos) {
            MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64, eosPts);
            NotifyEosLocked(eosPts);
            return;
        }
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
    FALSE_RETURN_V_MSG_E(nextFilter != nullptr, Status::ERROR_NULL_POINTER, "nextFilter is nullptr");
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<SurfaceEncoderFilterLinkCallback>(shared_from_this());
    std::shared_ptr<Meta> parameter;
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        nextFilter_ = nextFilter;
        nextFiltersMap_[outType].push_back(nextFilter_);
        parameter = configureParameter_;
    }
    nextFilter->OnLinked(outType, parameter, filterLinkCallback);
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
    onLinkedResultCallback_ = callback;
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
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        if (mediaCodec_ == nullptr) {
            return;
        }
        mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    }
    if (onLinkedResultCallback_) {
        onLinkedResultCallback_->OnLinkedResult(nullptr, meta);
    }
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
    std::lock_guard<std::mutex> lock(mediaCodecMutex_);
    if (mediaCodec_) {
        mediaCodec_->SetCallingInfo(appUid, appPid, bundleName, instanceId);
    }
}

void SurfaceEncoderFilter::OnReportKeyFramePts(std::string KeyFramePts)
{
    MEDIA_LOG_I("OnReportKeyFramePts %{public}s enter", KeyFramePts.c_str());
    const std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    userMeta->SetData("com.openharmony.recorder.timestamp", KeyFramePts);
    std::shared_ptr<Filter> nextFilter;
    {
        std::lock_guard<std::mutex> lock(mediaCodecMutex_);
        nextFilter = nextFilter_;
    }
    std::shared_ptr<MuxerFilter> muxerFilter = std::static_pointer_cast<MuxerFilter>(nextFilter);
    if (muxerFilter != nullptr) {
        muxerFilter->SetUserMeta(userMeta);
        MEDIA_LOG_I("SetUserMeta %{public}s", KeyFramePts.c_str());
    } else {
        MEDIA_LOG_E("muxerFilter is null");
    }
}

void SurfaceEncoderFilter::OnReportFirstFramePts(int64_t firstFramePts)
{
    MEDIA_LOG_I("OnReportFirstFramePts: " PUBLIC_LOG_D64, firstFramePts);
    if (eventReceiver_ != nullptr) {
        eventReceiver_->OnEvent({"surface_encoder_filter", EventType::EVENT_VIDEO_FIRST_FRAME, firstFramePts});
    }
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
