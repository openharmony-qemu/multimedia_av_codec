/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "video_resize_filter.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"

#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "detail_enhancer_video.h"
#include "detail_enhancer_video_common.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoResizeFilter" };
}

namespace OHOS {
namespace Media {
#ifdef USE_VIDEO_PROCESSING_ENGINE
using namespace VideoProcessingEngine;
#endif
namespace Pipeline {

static AutoRegisterFilter<VideoResizeFilter> g_registerVideoResizeFilter("builtin.transcoder.videoresize",
    FilterType::FILTERTYPE_VIDRESIZE,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<VideoResizeFilter>(name, FilterType::FILTERTYPE_VIDRESIZE);
    });

class VideoResizeFilterLinkCallback : public FilterLinkCallback {
public:
    explicit VideoResizeFilterLinkCallback(std::shared_ptr<VideoResizeFilter> videoResizeFilter)
        : videoResizeFilter_(std::move(videoResizeFilter))
    {
    }

    ~VideoResizeFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto resizeFilter = videoResizeFilter_.lock()) {
            resizeFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid resizeFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto resizeFilter = videoResizeFilter_.lock()) {
            resizeFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid resizeFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto resizeFilter = videoResizeFilter_.lock()) {
            resizeFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid resizeFilter");
        }
    }
private:
    std::weak_ptr<VideoResizeFilter> videoResizeFilter_;
};

#ifdef USE_VIDEO_PROCESSING_ENGINE
class ResizeDetailEnhancerVideoCallback : public DetailEnhancerVideoCallback {
public:
    explicit ResizeDetailEnhancerVideoCallback(std::shared_ptr<VideoResizeFilter> videoResizeFilter)
        : videoResizeFilter_(std::move(videoResizeFilter))
    {
    }

    void OnError(VPEAlgoErrCode errorCode) override
    {
    }

    void OnState(VPEAlgoState state) override
    {
    }

    void OnOutputBufferAvailable(uint32_t index, DetailEnhBufferFlag flag) override
    {
        if (auto videoResizeFilter = videoResizeFilter_.lock()) {
            videoResizeFilter->OnOutputBufferAvailable(index, static_cast<uint32_t>(flag));
            if (flag == DETAIL_ENH_BUFFER_FLAG_EOS) {
                videoResizeFilter->NotifyNextFilterEos();
            }
        } else {
            MEDIA_LOG_I("invalid videoResizeFilter");
        }
    }
private:
    std::weak_ptr<VideoResizeFilter> videoResizeFilter_;
};
#endif

VideoResizeFilter::VideoResizeFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    MEDIA_LOG_I("video resize filter create");
}

VideoResizeFilter::~VideoResizeFilter()
{
    MEDIA_LOG_I("video resize filter destroy");
}

Status VideoResizeFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    return Status::OK;
}

void VideoResizeFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
#ifdef USE_VIDEO_PROCESSING_ENGINE
    videoEnhancer_ = DetailEnhancerVideo::Create();
    if (videoEnhancer_ != nullptr) {
        std::shared_ptr<DetailEnhancerVideoCallback> detailEnhancerVideoCallback =
            std::make_shared<ResizeDetailEnhancerVideoCallback>(shared_from_this());
        videoEnhancer_->RegisterCallback(detailEnhancerVideoCallback);
    } else {
        MEDIA_LOG_I("Init videoEnhancer fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
        return;
    }
#else
    MEDIA_LOG_E("Init videoEnhancer fail, no VPE module");
    if (eventReceiver_) {
        eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return;
#endif
    if (!releaseBufferTask_) {
        releaseBufferTask_ = std::make_shared<Task>("VideoResize");
        releaseBufferTask_->RegisterJob([this] {
            ReleaseBuffer();
            return 0;
        });
    }
}

Status VideoResizeFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (videoEnhancer_ == nullptr) {
        MEDIA_LOG_E("Configure videoEnhancer is null");
        return Status::ERROR_NULL_POINTER;
    }
    const DetailEnhancerParameters parameter_ = {"", DetailEnhancerLevel::DETAIL_ENH_LEVEL_MEDIUM};
    int32_t ret = videoEnhancer_->SetParameter(parameter_, SourceType::VIDEO);
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer SetParameter fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
        return Status::ERROR_UNKNOWN;
    } else {
        return Status::OK;
    }
#else
    MEDIA_LOG_E("no VPE module");
    if (eventReceiver_) {
        eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return Status::ERROR_UNKNOWN;
#endif
}

sptr<Surface> VideoResizeFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (videoEnhancer_ == nullptr) {
        MEDIA_LOG_E("Configure videoEnhancer is null");
        return nullptr;
    }
    sptr<Surface> inputSurface = videoEnhancer_->GetInputSurface();
    if (inputSurface != nullptr) {
        inputSurface->SetDefaultUsage(BUFFER_USAGE_CPU_READ);
    }
    return inputSurface;
#else
    MEDIA_LOG_E("no VPE module");
    if (eventReceiver_) {
        eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return nullptr;
#endif
}

Status VideoResizeFilter::SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height)
{
    MEDIA_LOG_I("SetOutputSurface");
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (surface == nullptr) {
        MEDIA_LOG_E("SetOutputSurface surface is null");
        return Status::ERROR_NULL_POINTER;
    } else {
        surface->SetRequestWidthAndHeight(width, height);
    }
    if (videoEnhancer_ == nullptr) {
        MEDIA_LOG_E("Configure videoEnhancer is null");
        return Status::ERROR_NULL_POINTER;
    }
    int32_t ret = videoEnhancer_->SetOutputSurface(surface);
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer SetOutputSurface fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
#else
    MEDIA_LOG_E("no VPE module");
    if (eventReceiver_) {
        eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return Status::ERROR_UNKNOWN;
#endif
}

Status VideoResizeFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    if (filterCallback_ == nullptr) {
        return Status::ERROR_UNKNOWN;
    }
    switch (filterType_) {
        case FilterType::FILTERTYPE_VIDRESIZE:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_RAW_VIDEO);
            break;
        default:
            break;
    }
    return Status::OK;
}

Status VideoResizeFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    isThreadExit_ = false;
    if (releaseBufferTask_) {
        releaseBufferTask_->Start();
    }
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (videoEnhancer_ == nullptr) {
        MEDIA_LOG_E("DoStart videoEnhancer is null");
        return Status::ERROR_NULL_POINTER;
    }
    int32_t ret = videoEnhancer_->Start();
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer Start fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
#else
    MEDIA_LOG_E("no VPE module");
    if (eventReceiver_) {
        eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return Status::ERROR_UNKNOWN;
#endif
}

Status VideoResizeFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status VideoResizeFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status VideoResizeFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    if (releaseBufferTask_) {
        isThreadExit_ = true;
        releaseBufferCondition_.notify_all();
        releaseBufferTask_->Stop();
        MEDIA_LOG_I("releaseBufferTask_ Stop");
    }
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (!videoEnhancer_) {
        return Status::OK;
    }
    int32_t ret = videoEnhancer_->Stop();
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer Stop fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        }
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
#else
    MEDIA_LOG_E("no VPE module");
    if (eventReceiver_) {
        eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return Status::ERROR_UNKNOWN;
#endif
}

Status VideoResizeFilter::DoFlush()
{
    return Status::OK;
}

Status VideoResizeFilter::DoRelease()
{
    return Status::OK;
}

Status VideoResizeFilter::NotifyNextFilterEos()
{
    MEDIA_LOG_I("NotifyNextFilterEos");
    for (auto iter : nextFiltersMap_) {
        for (auto filter : iter.second) {
            std::shared_ptr<Meta> eosMeta = std::make_shared<Meta>();
            eosMeta->Set<Tag::MEDIA_END_OF_STREAM>(true);
            eosMeta->Set<Tag::USER_FRAME_PTS>(eosPts_);
            filter->SetParameter(eosMeta);
        }
    }
    return Status::OK;
}

void VideoResizeFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    bool isEos = false;
    if (parameter->Find(Tag::MEDIA_END_OF_STREAM) != parameter->end() &&
        parameter->Get<Tag::MEDIA_END_OF_STREAM>(isEos) &&
        parameter->Get<Tag::USER_FRAME_PTS>(eosPts_)) {
        if (isEos) {
#ifdef USE_VIDEO_PROCESSING_ENGINE
            MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64, eosPts_);
            if (videoEnhancer_ == nullptr) {
                MEDIA_LOG_E("videoEnhancer is null");
                return;
            }
            videoEnhancer_->NotifyEos();
#endif
            return;
        }
    }

#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (videoEnhancer_ == nullptr) {
        MEDIA_LOG_E("videoEnhancer is null");
        return;
    }
    const DetailEnhancerParameters parameter_ = {"", DetailEnhancerLevel::DETAIL_ENH_LEVEL_MEDIUM};
    int32_t ret = videoEnhancer_->SetParameter(parameter_, SourceType::VIDEO);
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer SetParameter fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR,
                MSERR_UNSUPPORT_VID_PARAMS});
        }
    }
#else
    MEDIA_LOG_E("no VPE module");
    eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR, MSERR_UNKNOWN});
#endif
}

void VideoResizeFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
}

Status VideoResizeFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<VideoResizeFilterLinkCallback>(shared_from_this());
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    if (ret != Status::OK && eventReceiver_) {
        eventReceiver_->OnEvent({"VideoResizeFilter::LinkNext error", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "OnLinked failed");
    return Status::OK;
}

Status VideoResizeFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status VideoResizeFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType VideoResizeFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

Status VideoResizeFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status VideoResizeFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status VideoResizeFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void VideoResizeFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult enter");
    if (onLinkedResultCallback_) {
        onLinkedResultCallback_->OnLinkedResult(nullptr, meta);
    }
}

void VideoResizeFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
    (void) meta;
}

void VideoResizeFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
    (void) meta;
}

void VideoResizeFilter::OnOutputBufferAvailable(uint32_t index, uint32_t flag)
{
    MEDIA_LOG_D("OnOutputBufferAvailable enter. index: %{public}u", index);
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        indexs_.push_back(std::make_pair(index, flag));
    }
    releaseBufferCondition_.notify_all();
}

void VideoResizeFilter::ReleaseBuffer()
{
    MEDIA_LOG_I("ReleaseBuffer");
    while (!isThreadExit_) {
        std::vector<std::pair<uint32_t, uint32_t>> indexs;
        {
            std::unique_lock<std::mutex> lock(releaseBufferMutex_);
            releaseBufferCondition_.wait(lock);
            indexs = indexs_;
            indexs_.clear();
        }
#ifdef USE_VIDEO_PROCESSING_ENGINE
        if (videoEnhancer_) {
            for (auto &index : indexs) {
                bool isRender = (index.second != static_cast<uint32_t>(DETAIL_ENH_BUFFER_FLAG_EOS));
                videoEnhancer_->ReleaseOutputBuffer(index.first, isRender);
            }
        }
#endif
    }
}

void VideoResizeFilter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    MEDIA_LOG_I("SetFaultEvent");
}

void VideoResizeFilter::SetFaultEvent(const std::string &errMsg)
{
    MEDIA_LOG_I("SetFaultEvent");
}

void VideoResizeFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS