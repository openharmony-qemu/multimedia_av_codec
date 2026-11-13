/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <chrono>
#include "filter/filter_factory.h"
#include "common/media_core.h"
#include "sync_fence.h"

#ifdef USE_VIDEO_PROCESSING_ENGINE
#include "detail_enhancer_video.h"
#include "detail_enhancer_video_common.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoResizeFilter" };
}

namespace OHOS {
namespace Media {
#ifdef USE_VIDEO_PROCESSING_ENGINE
using namespace VideoProcessingEngine;
constexpr int64_t VARIABLE_INCREMENT_INTERVAL = 1;
#endif
namespace Pipeline {

namespace {
constexpr uint64_t SOFTWARE_BUFFER_USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
constexpr int32_t SOFTWARE_BUFFER_TIMEOUT_MS = 3000;
constexpr int32_t SOFTWARE_FRAME_INTERVAL_MS = 15;
constexpr uint32_t INPUT_QUEUE_SIZE = 8;
constexpr int32_t STRIDE_ALIGNMENT = 16;
constexpr uint32_t CHROMA_SUBSAMPLE = 2;
constexpr uint32_t RGBA_PIXEL_BYTES = 4;

struct ResizePlane {
    uint8_t *data;
    int32_t width;
    int32_t height;
    uint32_t stride;
    uint32_t pixelBytes;
};

uint64_t GetPlaneOffset(const OH_NativeBuffer_Planes *planes, uint32_t index, uint64_t fallback)
{
    return planes != nullptr && planes->planeCount > index && planes->planes[index].offset != 0 ?
        planes->planes[index].offset : fallback;
}

uint32_t GetPlaneStride(const OH_NativeBuffer_Planes *planes, uint32_t index, uint32_t fallback)
{
    return planes != nullptr && planes->planeCount > index && planes->planes[index].rowStride != 0 ?
        planes->planes[index].rowStride : fallback;
}

bool AddResizePlane(const sptr<SurfaceBuffer> &buffer, std::vector<ResizePlane> &planes,
    uint64_t offset, int32_t width, int32_t height, uint32_t stride, uint32_t pixelBytes)
{
    uint64_t rowBytes = static_cast<uint64_t>(width) * pixelBytes;
    uint64_t size = buffer->GetSize();
    if (width <= 0 || height <= 0 || stride < rowBytes || offset > size ||
        static_cast<uint64_t>(height - 1) * stride + rowBytes > size - offset) {
        MEDIA_LOG_E("Invalid software resize plane layout");
        return false;
    }
    planes.push_back({static_cast<uint8_t *>(buffer->GetVirAddr()) + offset, width, height, stride, pixelBytes});
    return true;
}

bool GetResizePlanes(const sptr<SurfaceBuffer> &buffer, std::vector<ResizePlane> &planes)
{
    if (buffer == nullptr || buffer->GetVirAddr() == nullptr || buffer->GetWidth() <= 0 ||
        buffer->GetHeight() <= 0 || buffer->GetStride() <= 0) {
        return false;
    }
    int32_t width = buffer->GetWidth();
    int32_t height = buffer->GetHeight();
    uint32_t stride = static_cast<uint32_t>(buffer->GetStride());
    int32_t format = buffer->GetFormat();
    if (format != GRAPHIC_PIXEL_FMT_RGBA_8888 && format != GRAPHIC_PIXEL_FMT_YCBCR_420_SP &&
        format != GRAPHIC_PIXEL_FMT_YCRCB_420_SP && format != GRAPHIC_PIXEL_FMT_YCBCR_420_P) {
        MEDIA_LOG_E("Unsupported software resize format: " PUBLIC_LOG_D32, format);
        return false;
    }
    OH_NativeBuffer_Planes *nativePlanes = nullptr;
    if (buffer->GetPlanesInfo(reinterpret_cast<void **>(&nativePlanes)) != GSERROR_OK) {
        nativePlanes = nullptr;
    }
    uint32_t pixelBytes = format == GRAPHIC_PIXEL_FMT_RGBA_8888 ? RGBA_PIXEL_BYTES : 1;
    if (!AddResizePlane(buffer, planes, GetPlaneOffset(nativePlanes, 0, 0), width, height, stride, pixelBytes)) {
        return false;
    }
    if (format == GRAPHIC_PIXEL_FMT_RGBA_8888) {
        return true;
    }
    int32_t chromaWidth = width / CHROMA_SUBSAMPLE + width % CHROMA_SUBSAMPLE;
    int32_t chromaHeight = height / CHROMA_SUBSAMPLE + height % CHROMA_SUBSAMPLE;
    uint64_t uvOffset = static_cast<uint64_t>(stride) * height;
    if (format != GRAPHIC_PIXEL_FMT_YCBCR_420_P) {
        // Native plane 1 is U and plane 2 is V, including interleaved NV12/NV21 buffers.
        uint32_t uvPlane = format == GRAPHIC_PIXEL_FMT_YCRCB_420_SP ? CHROMA_SUBSAMPLE : 1;
        return AddResizePlane(buffer, planes, GetPlaneOffset(nativePlanes, uvPlane, uvOffset),
            chromaWidth, chromaHeight, GetPlaneStride(nativePlanes, uvPlane, stride), CHROMA_SUBSAMPLE);
    }
    uint32_t chromaStride = GetPlaneStride(nativePlanes, 1, (stride + 1) / CHROMA_SUBSAMPLE);
    uvOffset = GetPlaneOffset(nativePlanes, 1, uvOffset);
    uint64_t vOffset = uvOffset + static_cast<uint64_t>(chromaStride) * chromaHeight;
    return AddResizePlane(buffer, planes, uvOffset, chromaWidth, chromaHeight, chromaStride, 1) &&
        AddResizePlane(buffer, planes, GetPlaneOffset(nativePlanes, CHROMA_SUBSAMPLE, vOffset),
            chromaWidth, chromaHeight, GetPlaneStride(nativePlanes, CHROMA_SUBSAMPLE, chromaStride), 1);
}

bool ScaleSurfaceBuffer(const sptr<SurfaceBuffer> &srcBuffer, const sptr<SurfaceBuffer> &dstBuffer)
{
    std::vector<ResizePlane> srcPlanes;
    std::vector<ResizePlane> dstPlanes;
    if (srcBuffer->GetFormat() != dstBuffer->GetFormat() || !GetResizePlanes(srcBuffer, srcPlanes) ||
        !GetResizePlanes(dstBuffer, dstPlanes) || srcPlanes.size() != dstPlanes.size()) {
        return false;
    }
    for (size_t plane = 0; plane < srcPlanes.size(); ++plane) {
        const auto &src = srcPlanes[plane];
        const auto &dst = dstPlanes[plane];
        for (int32_t y = 0; y < dst.height; ++y) {
            auto *srcRow = src.data + (static_cast<uint64_t>(y) * src.height / dst.height) * src.stride;
            auto *dstRow = dst.data + static_cast<uint64_t>(y) * dst.stride;
            for (int32_t x = 0; x < dst.width; ++x) {
                uint64_t srcX = static_cast<uint64_t>(x) * src.width / dst.width;
                for (uint32_t byte = 0; byte < src.pixelBytes; ++byte) {
                    dstRow[static_cast<uint64_t>(x) * dst.pixelBytes + byte] =
                        srcRow[srcX * src.pixelBytes + byte];
                }
            }
        }
    }
    return true;
}

class VideoResizeSurfaceListener : public IBufferConsumerListener {
public:
    explicit VideoResizeSurfaceListener(std::function<void()> onBufferAvailable)
        : onBufferAvailable_(std::move(onBufferAvailable)) {}

    void OnBufferAvailable() override
    {
        onBufferAvailable_();
    }

private:
    std::function<void()> onBufferAvailable_;
};
}

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
        auto videoResizeFilter = videoResizeFilter_.lock();
        FALSE_RETURN_MSG(videoResizeFilter != nullptr, "invalid videoResizeFilter");
        videoResizeFilter->OnVPEError(static_cast<int32_t>(errorCode));
    }

    void OnState(VPEAlgoState state) override
    {
    }

    void OnOutputBufferAvailable(uint32_t index, DetailEnhBufferFlag flag) override
    {
        if (auto videoResizeFilter = videoResizeFilter_.lock()) {
            videoResizeFilter->OnOutputBufferAvailable(index, static_cast<uint32_t>(flag));
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
    DoRelease();
    MEDIA_LOG_I("video resize filter destroy");
}

bool VideoResizeFilter::IsSoftwareMode() const
{
#ifdef USE_VIDEO_PROCESSING_ENGINE
    return videoEnhancer_ == nullptr;
#else
    return true;
#endif
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
    eosPts_ = 0;
    frameNum_ = 0;
    currentFrameNum_ = 0;
    pendingInputBuffers_ = 0;
    eosReceived_ = false;
    eosNotified_ = false;
    isPaused_ = false;
    isThreadExit_ = true;
#ifdef USE_VIDEO_PROCESSING_ENGINE
    videoEnhancer_ = DetailEnhancerVideo::Create();
    if (videoEnhancer_ != nullptr) {
        std::shared_ptr<DetailEnhancerVideoCallback> detailEnhancerVideoCallback =
            std::make_shared<ResizeDetailEnhancerVideoCallback>(shared_from_this());
        videoEnhancer_->RegisterCallback(detailEnhancerVideoCallback);
    } else {
        MEDIA_LOG_I("Create videoEnhancer failed, using software resize");
    }
#else
    MEDIA_LOG_I("No VPE module, using software resize");
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
    if (parameter != nullptr) {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        parameter->Get<Tag::VIDEO_WIDTH>(outputWidth_);
        parameter->Get<Tag::VIDEO_HEIGHT>(outputHeight_);
    }
    if (IsSoftwareMode()) {
        return Status::OK;
    }
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
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR,
                static_cast<int32_t>(MSERR_UNSUPPORT_VID_PARAMS)});
        }
        return Status::ERROR_VID_ENHANCER_CONFIGURE_FAILED;
    } else {
        return Status::OK;
    }
#endif
    return Status::OK;
}

sptr<Surface> VideoResizeFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (videoEnhancer_ != nullptr) {
        return videoEnhancer_->GetInputSurface();
    }
#endif
    if (inputProducerSurface_ != nullptr) {
        return inputProducerSurface_;
    }
    auto consumer = Surface::CreateSurfaceAsConsumer("VideoResizeSurface");
    FALSE_RETURN_V_MSG(consumer != nullptr, nullptr, "Create input consumer surface failed");
    FALSE_RETURN_V_MSG(consumer->SetDefaultUsage(SOFTWARE_BUFFER_USAGE) == GSERROR_OK &&
        consumer->SetQueueSize(INPUT_QUEUE_SIZE) == GSERROR_OK, nullptr, "Configure input consumer surface failed");
    std::weak_ptr<VideoResizeFilter> weakFilter = shared_from_this();
    sptr<IBufferConsumerListener> listener = new VideoResizeSurfaceListener([weakFilter] {
        if (auto filter = weakFilter.lock()) {
            {
                std::lock_guard<std::mutex> lock(filter->releaseBufferMutex_);
                ++filter->pendingInputBuffers_;
            }
            filter->releaseBufferCondition_.notify_all();
        }
    });
    FALSE_RETURN_V_MSG(consumer->RegisterConsumerListener(listener) == GSERROR_OK, nullptr,
        "Register input consumer listener failed");
    auto producer = consumer->GetProducer();
    auto surface = Surface::CreateSurfaceAsProducer(producer);
    if (surface == nullptr) {
        consumer->UnregisterConsumerListener();
        return nullptr;
    }
    inputConsumerSurface_ = consumer;
    inputConsumerListener_ = listener;
    inputProducerSurface_ = surface;
    return inputProducerSurface_;
}

Status VideoResizeFilter::SetOutputSurface(sptr<Surface> surface, int32_t width, int32_t height)
{
    MEDIA_LOG_I("SetOutputSurface");
    if (surface == nullptr) {
        MEDIA_LOG_E("SetOutputSurface surface is null");
        return Status::ERROR_NULL_POINTER;
    }
    FALSE_RETURN_V_MSG(width > 0 && height > 0, Status::ERROR_INVALID_PARAMETER, "Invalid output dimensions");
    surface->SetRequestWidthAndHeight(width, height);
    if (IsSoftwareMode()) {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        outputSurface_ = surface;
        outputWidth_ = width;
        outputHeight_ = height;
        return Status::OK;
    }
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (videoEnhancer_ == nullptr) {
        MEDIA_LOG_E("Configure videoEnhancer is null");
        return Status::ERROR_NULL_POINTER;
    }
    int32_t ret = videoEnhancer_->SetOutputSurface(surface);
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer SetOutputSurface fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR,
                static_cast<int32_t>(MSERR_UNKNOWN)});
        }
        return Status::ERROR_UNKNOWN;
    }
#endif
    return Status::OK;
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
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        isThreadExit_ = false;
        isPaused_ = false;
        eosReceived_ = false;
        eosNotified_ = false;
        currentFrameNum_ = 0;
    }
    if (releaseBufferTask_) {
        releaseBufferTask_->Start();
    }
    if (IsSoftwareMode()) {
        return Status::OK;
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
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR,
                static_cast<int32_t>(MSERR_UNKNOWN)});
        }
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
#endif
    return Status::OK;
}

Status VideoResizeFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        isPaused_ = true;
    }
    releaseBufferCondition_.notify_all();
    return Status::OK;
}

Status VideoResizeFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        isPaused_ = false;
    }
    releaseBufferCondition_.notify_all();
    return Status::OK;
}

Status VideoResizeFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        isThreadExit_ = true;
    }
    releaseBufferCondition_.notify_all();
    if (releaseBufferTask_) {
        releaseBufferTask_->Stop();
        MEDIA_LOG_I("releaseBufferTask_ Stop");
    }
    if (IsSoftwareMode()) {
        DrainInputBuffers();
        return Status::OK;
    }
#ifdef USE_VIDEO_PROCESSING_ENGINE
    if (!videoEnhancer_) {
        return Status::OK;
    }
    int32_t ret = videoEnhancer_->Stop();
    if (ret != 0) {
        MEDIA_LOG_E("videoEnhancer Stop fail");
        if (eventReceiver_) {
            eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR,
                static_cast<int32_t>(MSERR_UNKNOWN)});
        }
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
#endif
    return Status::OK;
}

Status VideoResizeFilter::DoFlush()
{
    return Status::OK;
}

Status VideoResizeFilter::DoRelease()
{
    if (IsSoftwareMode()) {
        DoStop();
        if (inputConsumerSurface_ != nullptr) {
            inputConsumerSurface_->UnregisterConsumerListener();
        }
        inputConsumerListener_ = nullptr;
        inputProducerSurface_ = nullptr;
        inputConsumerSurface_ = nullptr;
        outputSurface_ = nullptr;
        releaseBufferTask_.reset();
    }
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
    FALSE_RETURN_MSG(parameter != nullptr, "parameter is nullptr");
    if (IsSoftwareMode()) {
        if (parameter->Get<Tag::MEDIA_END_OF_STREAM>(isEos) && isEos) {
            {
                std::lock_guard<std::mutex> lock(releaseBufferMutex_);
                parameter->Get<Tag::USER_FRAME_PTS>(eosPts_);
                parameter->Get<Tag::USER_PUSH_DATA_TIME>(frameNum_);
                eosReceived_ = true;
            }
            releaseBufferCondition_.notify_all();
        }
        return;
    }
    if (parameter->Find(Tag::MEDIA_END_OF_STREAM) != parameter->end() &&
        parameter->Get<Tag::MEDIA_END_OF_STREAM>(isEos) &&
        parameter->Get<Tag::USER_FRAME_PTS>(eosPts_) &&
        parameter->Get<Tag::USER_PUSH_DATA_TIME>(frameNum_)) {
        if (isEos) {
#ifdef USE_VIDEO_PROCESSING_ENGINE
            MEDIA_LOG_I("lastBuffer PTS: " PUBLIC_LOG_D64 " frameNum: " PUBLIC_LOG_D64,
                eosPts_, frameNum_);
            if (videoEnhancer_ == nullptr) {
                MEDIA_LOG_E("videoEnhancer is null");
                return;
            }
            MEDIA_LOG_I("currentFrameNum: " PUBLIC_LOG_D64 " frameNum: " PUBLIC_LOG_D64,
                currentFrameNum_.load(), frameNum_);
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
                static_cast<int32_t>(MSERR_UNSUPPORT_VID_PARAMS)});
        }
    }
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
        eventReceiver_->OnEvent({"VideoResizeFilter::LinkNext error", EventType::EVENT_ERROR,
            static_cast<int32_t>(MSERR_UNKNOWN)});
    }
    return ret;
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
    MEDIA_LOG_D("OnOutputBufferAvailable enter. index: " PUBLIC_LOG_U32, index);
#ifdef USE_VIDEO_PROCESSING_ENGINE
    {
        std::lock_guard<std::mutex> lock(releaseBufferMutex_);
        indexs_.push_back(index);
        if (flag != static_cast<uint32_t>(DETAIL_ENH_BUFFER_FLAG_EOS)) {
            currentFrameNum_.fetch_add(VARIABLE_INCREMENT_INTERVAL, std::memory_order_relaxed);
        } else {
            eosBufferIndex_ = index;
        }
    }
    releaseBufferCondition_.notify_all();
#endif
}

void VideoResizeFilter::ReleaseBuffer()
{
    MEDIA_LOG_I("ReleaseBuffer");
    if (IsSoftwareMode()) {
        ReleaseSoftwareBuffers();
        return;
    }
    while (!isThreadExit_) {
        std::vector<uint32_t> indexs;
        {
            std::unique_lock<std::mutex> lock(releaseBufferMutex_);
            releaseBufferCondition_.wait(lock, [this] {
                return isThreadExit_ || !indexs_.empty();
            });
            indexs = indexs_;
            indexs_.clear();
        }
#ifdef USE_VIDEO_PROCESSING_ENGINE
        if (videoEnhancer_) {
            ReleaseOutputBuffer(indexs);
        }
#endif
    }
}

void VideoResizeFilter::ReleaseSoftwareBuffers()
{
    std::unique_lock<std::mutex> lock(releaseBufferMutex_);
    while (!isThreadExit_) {
        releaseBufferCondition_.wait(lock, [this] {
            return isThreadExit_ || (!isPaused_ &&
                (pendingInputBuffers_ > 0 || (eosReceived_ && !eosNotified_)));
        });
        while (!isThreadExit_ && !isPaused_) {
            // Acquire outside the mutex: a producer may be dispatching OnBufferAvailable.
            if (pendingInputBuffers_ > 0) {
                --pendingInputBuffers_;
            }
            lock.unlock();
            sptr<SurfaceBuffer> srcBuffer;
            sptr<SyncFence> fence;
            int64_t pts = 0;
            Rect damage = {};
            GSError ret = inputConsumerSurface_ == nullptr ? GSERROR_INVALID_ARGUMENTS :
                inputConsumerSurface_->AcquireBuffer(srcBuffer, fence, pts, damage);
            if (ret != GSERROR_OK || srcBuffer == nullptr) {
                lock.lock();
                if (pendingInputBuffers_ > 0) {
                    continue;
                }
                if (!isThreadExit_ && !isPaused_ && eosReceived_ && !eosNotified_) {
                    // The queue is drained, even if the decoder's reported frame count differs.
                    eosNotified_ = true;
                    lock.unlock();
                    NotifyNextFilterEos();
                    lock.lock();
                }
                break;
            }
            if (fence == nullptr || !fence->IsValid() || fence->Wait(SOFTWARE_BUFFER_TIMEOUT_MS) == 0) {
                srcBuffer->InvalidateCache();
                ResizeBuffer(srcBuffer, pts);
            } else {
                MEDIA_LOG_E("Wait for input buffer fence failed");
            }
            inputConsumerSurface_->ReleaseBuffer(srcBuffer, -1);
            lock.lock();
            ++currentFrameNum_;
            releaseBufferCondition_.wait_for(lock, std::chrono::milliseconds(SOFTWARE_FRAME_INTERVAL_MS), [this] {
                return isThreadExit_ || isPaused_;
            });
        }
    }
}

void VideoResizeFilter::ResizeBuffer(sptr<SurfaceBuffer> &srcBuffer, int64_t pts)
{
    std::unique_lock<std::mutex> lock(releaseBufferMutex_);
    releaseBufferCondition_.wait(lock, [this] { return isThreadExit_ || !isPaused_; });
    if (isThreadExit_ || outputSurface_ == nullptr || outputWidth_ <= 0 || outputHeight_ <= 0) {
        return;
    }
    auto outputSurface = outputSurface_;
    BufferRequestConfig requestConfig = {};
    requestConfig.width = outputWidth_;
    requestConfig.height = outputHeight_;
    requestConfig.strideAlignment = STRIDE_ALIGNMENT;
    requestConfig.format = srcBuffer->GetFormat();
    requestConfig.usage = SOFTWARE_BUFFER_USAGE;
    requestConfig.timeout = SOFTWARE_BUFFER_TIMEOUT_MS;
    lock.unlock();
    sptr<SurfaceBuffer> dstBuffer;
    sptr<SyncFence> dstFence;
    if (outputSurface->RequestBuffer(dstBuffer, dstFence, requestConfig) != GSERROR_OK || dstBuffer == nullptr) {
        MEDIA_LOG_E("Request software resize output buffer failed");
        return;
    }
    if ((dstFence != nullptr && dstFence->IsValid() && dstFence->Wait(SOFTWARE_BUFFER_TIMEOUT_MS) != 0) ||
        !ScaleSurfaceBuffer(srcBuffer, dstBuffer)) {
        MEDIA_LOG_E("Scale software resize buffer failed");
        outputSurface->CancelBuffer(dstBuffer);
        return;
    }
    dstBuffer->FlushCache();
    lock.lock();
    releaseBufferCondition_.wait(lock, [this] { return isThreadExit_ || !isPaused_; });
    if (isThreadExit_) {
        lock.unlock();
        outputSurface->CancelBuffer(dstBuffer);
        return;
    }
    BufferFlushConfig flushConfig = {};
    flushConfig.damage = {0, 0, requestConfig.width, requestConfig.height};
    flushConfig.timestamp = pts;
    flushConfig.desiredPresentTimestamp = -1;
    lock.unlock();
    if (outputSurface->FlushBuffer(dstBuffer, -1, flushConfig) != GSERROR_OK) {
        outputSurface->CancelBuffer(dstBuffer);
    }
}

void VideoResizeFilter::DrainInputBuffers()
{
    if (inputConsumerSurface_ != nullptr) {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int64_t pts = 0;
        Rect damage = {};
        while (inputConsumerSurface_->AcquireBuffer(buffer, fence, pts, damage) == GSERROR_OK) {
            if (buffer != nullptr) {
                inputConsumerSurface_->ReleaseBuffer(buffer, -1);
            }
        }
    }
    std::lock_guard<std::mutex> lock(releaseBufferMutex_);
    pendingInputBuffers_ = 0;
}

#ifdef USE_VIDEO_PROCESSING_ENGINE
void VideoResizeFilter::ReleaseOutputBuffer(std::vector<uint32_t> &indexs)
{
    for (auto &index : indexs) {
        if (index != eosBufferIndex_) {
            videoEnhancer_->ReleaseOutputBuffer(index, true);
        } else {
            videoEnhancer_->ReleaseOutputBuffer(index, false);
            NotifyNextFilterEos();
        }
    }
}
#endif

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

#ifdef USE_VIDEO_PROCESSING_ENGINE
void VideoResizeFilter::OnVPEError(int32_t errorCode)
{
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "no eventReceiver_");
    FALSE_RETURN_NOLOG(isVPEReportError_ == false);
    isVPEReportError_ = true;
    eventReceiver_->OnEvent({"video_resize_filter", EventType::EVENT_ERROR,
        static_cast<int32_t>(MSERR_VID_RESIZE_FAILED)});
}
#endif
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS