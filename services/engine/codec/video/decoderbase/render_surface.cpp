/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "render_surface.h"
#include "avcodec_trace.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "RenderSurface"};
constexpr uint32_t INDEX_INPUT = 0;
constexpr uint32_t INDEX_OUTPUT = 1;

int32_t RenderSurface::ReplaceOutputSurfaceWhenRunning(sptr<Surface> newSurface)
{
    CHECK_AND_RETURN_RET_LOG(sInfo_.surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT,
                             "Not support convert from AVBuffer Mode to Surface Mode");
    sptr<Surface> curSurface = sInfo_.surface;
    uint64_t oldId = curSurface->GetUniqueId();
    uint64_t newId = newSurface->GetUniqueId();
    AVCODEC_LOGI("Begin to switch surface %{public}" PRIu64 " -> %{public}" PRIu64 ".", oldId, newId);
    if (oldId == newId) {
        return AVCS_ERR_OK;
    }
    int32_t ret = RegisterListenerToSurface(newSurface);
    CHECK_AND_RETURN_RET_LOG(ret == GSERROR_OK, ret,
        "surface %{public}" PRIu64 ", RegisterListenerToSurface failed, GSError=%{public}d", newId, ret);
    int32_t outputBufferCnt = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outputBufferCnt);
    ret = SetQueueSize(newSurface, outputBufferCnt);
    if (ret != AVCS_ERR_OK) {
        UnRegisterListenerToSurface(newSurface);
        return ret;
    }
    std::unique_lock<std::mutex> sLock(surfaceMutex_);
    ret = SwitchBetweenSurface(newSurface);
    if (ret != AVCS_ERR_OK) {
        UnRegisterListenerToSurface(newSurface);
        sInfo_.surface = curSurface;
        CombineConsumerUsage();
        return ret;
    }
    sLock.unlock();
    AVCODEC_LOGI("Switch surface %{public}" PRIu64 " -> %{public}" PRIu64 ".", oldId, newId);
    return AVCS_ERR_OK;
}

void RenderSurface::UnRegisterListenerToSurface(const sptr<Surface> &surface)
{
    CHECK_AND_RETURN_LOG(surface != nullptr, "Surface is null, not need to unregister listener.");
    SurfaceTools::GetInstance().ReleaseSurface(instanceId_, surface, false);
}

void RenderSurface::CombineConsumerUsage() const
{
    uint64_t defaultUsage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
    uint64_t consumerUsage = sInfo_.surface->GetDefaultUsage();
    uint64_t cfgedUsage = sInfo_.requestConfig.usage;
    uint64_t finalUsage = defaultUsage | consumerUsage | cfgedUsage;
    sInfo_.requestConfig.usage = finalUsage;
    AVCODEC_LOGI("Usage: default(0x%{public}" PRIu64 ") | consumer(0x%{public}" PRIu64 ") | cfged(0x%{public}" PRIu64
                 ") -> final(0x%{public}" PRIu64 ").",
                 defaultUsage, consumerUsage, cfgedUsage, finalUsage);
}

int32_t RenderSurface::RegisterListenerToSurface(const sptr<Surface> &surface)
{
    uint64_t surfaceId = surface->GetUniqueId();
    wptr<RenderSurface> wp = this;
    bool ret =
        SurfaceTools::GetInstance().RegisterReleaseListener(instanceId_, surface,
            [wp, surfaceId](sptr<SurfaceBuffer> &) {
                sptr<RenderSurface> codec = wp.promote();
                if (!codec) {
                    AVCODEC_LOGD("decoder is nullptr");
                    return GSERROR_OK;
                }
                return codec->BufferReleasedByConsumer(surfaceId);
            });
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_UNKNOWN, "surface(%" PRIu64 ") register listener failed", surfaceId);
    StartRequestSurfaceBufferThread();
    return AVCS_ERR_OK;
}

int32_t RenderSurface::SetQueueSize(const sptr<Surface> &surface, uint32_t targetSize)
{
    uint64_t surfaceId = surface->GetUniqueId();
    int32_t err = surface->SetQueueSize(targetSize);
    CHECK_AND_RETURN_RET_LOG(err == 0, err,
                             "Surface(%{public}" PRIu64 ") set queue size %{public}u failed, GSError=%{public}d",
                             surfaceId, targetSize, err);
    AVCODEC_LOGI("Surface(%{public}" PRIu64 ") set queue size %{public}u succss.", surfaceId, targetSize);
    return AVCS_ERR_OK;
}

void RenderSurface::StartRequestSurfaceBufferThread()
{
    std::lock_guard<std::mutex> lck(requestBufferMutex_);
    if (!mRequestSurfaceBufferThread_.joinable()) {
        requestBufferThreadExit_ = false;
        requestBufferFinished_ = true;
        mRequestSurfaceBufferThread_ = std::thread(&RenderSurface::RequestSurfaceBufferThread, this);
    }
}

int32_t RenderSurface::SwitchBetweenSurface(const sptr<Surface> &newSurface)
{
    sptr<Surface> curSurface = sInfo_.surface;
    newSurface->Connect(); // cleancache will work only if the surface is connected by us
    newSurface->CleanCache(); // make sure new surface is empty
    std::vector<uint32_t> ownedBySurfaceBufferIndex;
    uint64_t newId = newSurface->GetUniqueId();
    for (uint32_t index = 0; index < buffers_[INDEX_OUTPUT].size(); index++) {
        auto surfaceMemory = buffers_[INDEX_OUTPUT][index]->sMemory;
        if (surfaceMemory == nullptr) {
            continue;
        }
        sptr<SurfaceBuffer> surfaceBuffer = nullptr;
        if (buffers_[INDEX_OUTPUT][index]->owner_ == Owner::OWNED_BY_SURFACE) {
            if (renderSurfaceBufferMap_.count(index)) {
                surfaceBuffer = renderSurfaceBufferMap_[index].first;
                ownedBySurfaceBufferIndex.push_back(index);
            }
        } else {
            RequestSurfaceBufferOnce(index);
            surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        }
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Get old surface buffer error!");
        int32_t err = newSurface->AttachBufferToQueue(surfaceBuffer);
        if (err != 0) {
            AVCODEC_LOGE("surface %{public}" PRIu64 ", AttachBufferToQueue(seq=%{public}u) failed, GSError=%{public}d",
                newId, surfaceBuffer->GetSeqNum(), err);
            return AVCS_ERR_UNKNOWN;
        }
        buffers_[INDEX_OUTPUT][index]->sMemory->isAttached = true;
    }
    int32_t val32 = 0;
    if (format_.ContainKey(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE)) {
        CHECK_AND_RETURN_RET_LOG(format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val32) && val32 >= 0 &&
                                 val32 <= static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_270),
                                 AVCS_ERR_INVALID_VAL, "Invalid rotation angle %{public}d", val32);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(val32)));
    }
    sInfo_.surface = newSurface;
    CombineConsumerUsage();
    for (uint32_t index: ownedBySurfaceBufferIndex) {
        int32_t ret = RenderNewSurfaceWithOldBuffer(newSurface, index);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Old surface buffer render failed!");
    }
    UnRegisterListenerToSurface(curSurface);
    curSurface->CleanCache(true); // make sure old surface is empty and go black
    return AVCS_ERR_OK;
}

bool RenderSurface::RequestSurfaceBufferOnce(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(!requestBufferThreadExit_.load(), false, "request surfacebuffer thread exited!");
    std::unique_lock<std::mutex> lck(requestBufferMutex_);
    requestSucceed_ = false;
    requestBufferFinished_ = false;
    requestSurfaceBufferQue_->Push(index);
    requestBufferCV_.notify_one();
    requestBufferOnceDoneCV_.wait(lck, [this]() { return requestBufferFinished_.load(); });
    CHECK_AND_RETURN_RET_LOG(requestSucceed_.load(), false, "Output surface memory %{public}u allocate failed", index);
    return true;
}

int32_t RenderSurface::RenderNewSurfaceWithOldBuffer(const sptr<Surface> &newSurface, uint32_t index)
{
    std::shared_ptr<FSurfaceMemory> surfaceMemory = buffers_[INDEX_OUTPUT][index]->sMemory;
    sptr<SurfaceBuffer> surfaceBuffer = renderSurfaceBufferMap_[index].first;
    OHOS::BufferFlushConfig flushConfig = renderSurfaceBufferMap_[index].second;
    if (sInfo_.scalingMode) {
        newSurface->SetScalingMode(surfaceBuffer->GetSeqNum(), sInfo_.scalingMode.value());
    }
    auto res = newSurface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("Failed to update surface memory: %{public}d", res);
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}


int32_t RenderSurface::FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, uint32_t index)
{
    RequestSurfaceBufferOnce(index);
    sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Get surface buffer failed!");
    if (!surfaceMemory->isAttached) {
        int32_t ret = Attach(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer attach failed!");
        surfaceMemory->isAttached = true;
    }
    OHOS::BufferFlushConfig flushConfig = {{0, 0, surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight()},
        outAVBuffer4Surface_[index]->pts_, -1};
    if (outAVBuffer4Surface_[index]->meta_->Find(OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP) !=
        outAVBuffer4Surface_[index]->meta_->end()) {
        outAVBuffer4Surface_[index]->meta_->Get<OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP>(
            flushConfig.desiredPresentTimestamp);
        outAVBuffer4Surface_[index]->meta_->Remove(OHOS::Media::Tag::VIDEO_DECODER_DESIRED_PRESENT_TIMESTAMP);
    }
    auto res = sInfo_.surface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    if (res == GSERROR_BUFFER_NOT_INCACHE) {
        AVCODEC_LOGW("Surface(%{public}" PRIu64 "), flush buffer(seq=%{public}u) failed, try to recover",
                     sInfo_.surface->GetUniqueId(), surfaceBuffer->GetSeqNum());
        int32_t ret = ClearSurfaceAndSetQueueSize(sInfo_.surface, outputBufferCnt_);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Clean surface and set queue size failed!");
        ret = Attach(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Surface buffer attach failed!");
        surfaceMemory->isAttached = true;
        res = sInfo_.surface->FlushBuffer(surfaceBuffer, -1, flushConfig);
    }
    surfaceMemory->owner = Owner::OWNED_BY_SURFACE;
    if (res != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGW("Failed to update surface memory: %{public}d", res);
        return AVCS_ERR_UNKNOWN;
    }
    renderSurfaceBufferMap_[index] = std::make_pair(surfaceBuffer, flushConfig);
    renderAvailQue_->Push(index);
    return AVCS_ERR_OK;
}

int32_t RenderSurface::Attach(sptr<SurfaceBuffer> surfaceBuffer)
{
    int32_t err = sInfo_.surface->AttachBufferToQueue(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(
        err == 0, err, "Surface(%{public}" PRIu64 "), attach buffer(%{public}u) to queue failed, GSError=%{public}d",
        sInfo_.surface->GetUniqueId(), surfaceBuffer->GetSeqNum(), err);
    return AVCS_ERR_OK;
}

int32_t RenderSurface::ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, int32_t bufferCnt)
{
    surface->Connect();
    surface->CleanCache(); // clean cache will work only if the surface is connected by us.
    int32_t ret = SetQueueSize(surface, bufferCnt);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set surface queue size failed!");
    ret = SetSurfaceCfg();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Set surface cfg failed!");
    CHECK_AND_RETURN_RET_LOGD(buffers_[INDEX_OUTPUT].size() > 0u, AVCS_ERR_OK, "Set surface cfg & queue size success.");
    int32_t valBufferCnt = 0;
    for (auto &it : buffers_[INDEX_OUTPUT]) {
        std::shared_ptr<FSurfaceMemory> surfaceMemory = it->sMemory;
        surfaceMemory->isAttached = false;
        valBufferCnt++;
    }
    CHECK_AND_RETURN_RET_LOG(valBufferCnt == bufferCnt, AVCS_ERR_UNKNOWN, "Outbuf cnt(%{public}d) != %{public}d",
                             valBufferCnt, bufferCnt);
    return AVCS_ERR_OK;
}

int32_t RenderSurface::SetSurfaceCfg()
{
    if (outputPixelFmt_ == VideoPixelFormat::UNKNOWN) {
        format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    }
    int32_t val32 = 0;
    format_.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, val32);
    outputPixelFmt_ = static_cast<VideoPixelFormat>(val32);
    
    GraphicPixelFormat surfacePixelFmt = TranslateSurfaceFormat(outputPixelFmt_);
    CHECK_AND_RETURN_RET_LOG(surfacePixelFmt != GraphicPixelFormat::GRAPHIC_PIXEL_FMT_BUTT, AVCS_ERR_UNSUPPORT,
                             "Failed to allocate output buffer: unsupported surface format");
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, static_cast<int32_t>(surfacePixelFmt));
    sInfo_.requestConfig.width = width_;
    sInfo_.requestConfig.height = height_;
    sInfo_.requestConfig.format = surfacePixelFmt;
    CHECK_AND_RETURN_RET_LOGD(sInfo_.surface != nullptr, AVCS_ERR_OK, "Buffer mode not need to set surface config.");
    if (format_.ContainKey(MediaDescriptionKey::MD_KEY_SCALE_TYPE)) {
        CHECK_AND_RETURN_RET_LOG(format_.GetIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, val32) && val32 >= 0 &&
                                 val32 <= static_cast<int32_t>(ScalingMode::SCALING_MODE_SCALE_FIT),
                                 AVCS_ERR_INVALID_VAL, "Invalid scaling mode %{public}d", val32);
        sInfo_.scalingMode = static_cast<ScalingMode>(val32);
        sInfo_.surface->SetScalingMode(sInfo_.scalingMode.value());
    }
    if (format_.ContainKey(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE)) {
        CHECK_AND_RETURN_RET_LOG(format_.GetIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, val32) && val32 >= 0 &&
                                 val32 <= static_cast<int32_t>(VideoRotation::VIDEO_ROTATION_270),
                                 AVCS_ERR_INVALID_VAL, "Invalid rotation angle %{public}d", val32);
        sInfo_.surface->SetTransform(TranslateSurfaceRotation(static_cast<VideoRotation>(val32)));
    }
    return AVCS_ERR_OK;
}

GSError RenderSurface::BufferReleasedByConsumer(uint64_t surfaceId)
{
    CHECK_AND_RETURN_RET_LOG(state_ == State::RUNNING || state_ == State::EOS,
        GSERROR_NO_PERMISSION, "In valid state_");
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    CHECK_AND_RETURN_RET_LOG(renderAvailQue_->Size() > 0, GSERROR_NO_BUFFER, "No available buffer");
    CHECK_AND_RETURN_RET_LOG(surfaceId == sInfo_.surface->GetUniqueId(), GSERROR_INVALID_ARGUMENTS,
                             "Ignore callback from old surface");
    RequestBufferFromConsumer();
    return GSERROR_OK;
}

void RenderSurface::RequestBufferFromConsumer()
{
    auto index = renderAvailQue_->Front();
    RequestSurfaceBufferOnce(index);
    std::shared_ptr<CodecBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
    std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
    auto queSize = renderAvailQue_->Size();
    uint32_t curIndex = 0;
    uint32_t i = 0;
    for (i = 0; i < queSize; i++) {
        curIndex = renderAvailQue_->Pop();
        if (surfaceMemory->GetBase() == buffers_[INDEX_OUTPUT][curIndex]->avBuffer->memory_->GetAddr() &&
            surfaceMemory->GetSize() == buffers_[INDEX_OUTPUT][curIndex]->avBuffer->memory_->GetCapacity()) {
            buffers_[INDEX_OUTPUT][index]->sMemory = buffers_[INDEX_OUTPUT][curIndex]->sMemory;
            buffers_[INDEX_OUTPUT][curIndex]->sMemory = surfaceMemory;
            break;
        } else {
            renderAvailQue_->Push(curIndex);
        }
    }
    if (i == queSize) {
        curIndex = index;
        outputBuffer->avBuffer = AVBuffer::CreateAVBuffer(surfaceMemory->GetBase(), surfaceMemory->GetSize());
        outputBuffer->width = width_;
        outputBuffer->height = height_;
        FindAvailIndex(curIndex);
    }
    buffers_[INDEX_OUTPUT][curIndex]->owner_ = Owner::OWNED_BY_CODEC;
    codecAvailQue_->Push(curIndex);
    if (renderSurfaceBufferMap_.count(curIndex)) {
        renderSurfaceBufferMap_.erase(curIndex);
    }
    AVCODEC_LOGD("Request output buffer success, index = %{public}u, queSize=%{public}zu, i=%{public}d", curIndex,
                 queSize, i);
}

void RenderSurface::FindAvailIndex(uint32_t index) const
{
    uint32_t curQueSize = renderAvailQue_->Size();
    for (uint32_t i = 0u; i < curQueSize; i++) {
        uint32_t num = renderAvailQue_->Pop();
        if (num == index) {
            break;
        } else {
            renderAvailQue_->Push(num);
        }
    }
}

void RenderSurface::RequestSurfaceBufferThread()
{
    while (!requestBufferThreadExit_.load()) {
        std::unique_lock<std::mutex> lck(requestBufferMutex_);
        requestBufferCV_.wait(lck, [this]() {
            return requestBufferThreadExit_.load() || !requestBufferFinished_.load();
        });
        if (requestBufferThreadExit_.load()) {
            requestBufferFinished_ = true;
            requestBufferOnceDoneCV_.notify_one();
            break;
        }
        auto index = requestSurfaceBufferQue_->Front();
        requestSurfaceBufferQue_->Pop();
        std::shared_ptr<CodecBuffer> outputBuffer = buffers_[INDEX_OUTPUT][index];
        std::shared_ptr<FSurfaceMemory> surfaceMemory = outputBuffer->sMemory;
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        if (surfaceBuffer == nullptr) {
            AVCODEC_LOGE("Get surface buffer failed, index=%{public}u", index);
        } else {
            requestSucceed_ = true;
        }
        requestBufferFinished_ = true;
        requestBufferOnceDoneCV_.notify_one();
    }
    AVCODEC_LOGI("RequestSurfaceBufferThread exit.");
}

void RenderSurface::StopRequestSurfaceBufferThread()
{
    if (mRequestSurfaceBufferThread_.joinable()) {
        requestBufferThreadExit_ = true;
        requestBufferFinished_ = false;
        requestBufferCV_.notify_all();
        requestBufferFinished_ = true;
        requestBufferOnceDoneCV_.notify_all();
        mRequestSurfaceBufferThread_.join();
    }
}

int32_t RenderSurface::FreezeBuffers(State curState)
{
    CHECK_AND_RETURN_RET_LOGD(state_ != State::FROZEN, AVCS_ERR_OK, "Video codec had been frozen!");
    std::lock_guard<std::mutex> sLock(surfaceMutex_);
    int32_t ret = SwapOutBuffers(INDEX_INPUT, curState);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Input buffers swap out failed!");
    ret = SwapOutBuffers(INDEX_OUTPUT, curState);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Output buffers swap out failed!");
    AVCODEC_LOGI("Freeze buffers success");
    return AVCS_ERR_OK;
}

int32_t RenderSurface::ActiveBuffers()
{
    CHECK_AND_RETURN_RET_LOGD(state_ == State::FREEZING || state_ == State::FROZEN, AVCS_ERR_INVALID_STATE,
                              "Only freezing or frozen state_ can swap in dma buffer!");
    int32_t ret = SwapInBuffers(INDEX_INPUT);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Input buffers swap in failed!");
    ret = SwapInBuffers(INDEX_OUTPUT);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Output buffers swap in failed!");
    AVCODEC_LOGI("Active buffers success");
    return AVCS_ERR_OK;
}

bool RenderSurface::CanSwapOut(bool isOutputBuffer, const std::shared_ptr<CodecBuffer> &codecBuffer)
{
    if (!isOutputBuffer) {
        AVCODEC_LOGE("Current buffers unsupport.");
        return false;
    }
    std::shared_ptr<FSurfaceMemory> surfaceMemory = codecBuffer->sMemory;
    CHECK_AND_RETURN_RET_LOGD(surfaceMemory != nullptr, false, "Current buffer->sMemory error!");
    Owner ownerValue = surfaceMemory->owner;
    AVCODEC_LOGD(
    "Buffer type: [%{public}u], codecBuffer->owner_: [%{public}d], "
    "codecBuffer->hasSwapedOut: [%{public}d].",
    isOutputBuffer, ownerValue, codecBuffer->hasSwapedOut);
    return !(ownerValue == Owner::OWNED_BY_SURFACE || codecBuffer->hasSwapedOut);
}

int32_t RenderSurface::SwapOutBuffers(bool isOutputBuffer, State curState) const
{
    uint32_t bufferType = isOutputBuffer ? INDEX_OUTPUT : INDEX_INPUT;
    CHECK_AND_RETURN_RET_LOGD(bufferType == INDEX_OUTPUT, AVCS_ERR_OK, "Input buffers can't be swapped out!");
    for (uint32_t i = 0u; i < buffers_[bufferType].size(); i++) {
        std::shared_ptr<CodecBuffer> codecBuffer = buffers_[bufferType][i];
        if (!CanSwapOut(isOutputBuffer, codecBuffer)) {
            AVCODEC_LOGW("Buf: [%{public}u] can't freeze, owner: [%{public}d] swaped out: [%{public}d]!", i,
                         codecBuffer->owner_.load(), codecBuffer->hasSwapedOut);
            continue;
        }
        std::shared_ptr<FSurfaceMemory> surfaceMemory = codecBuffer->sMemory;
        CHECK_AND_RETURN_RET_LOG(surfaceMemory != nullptr, AVCS_ERR_UNKNOWN, "Buf[%{public}u]->sMemory error!", i);
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AVCS_ERR_UNKNOWN, "Buf[%{public}u]->surfaceBuf error!", i);
        int32_t fd = surfaceBuffer->GetFileDescriptor();
        int32_t ret = DmaSwaper::GetInstance().SwapOutDma(pid_, fd);
        if (ret != AVCS_ERR_OK) {
            AVCODEC_LOGE("Buffer type[%{public}u] bufferId[%{public}u], fd[%{public}d], pid[%{public}d] freeze failed!",
                         bufferType, i, fd, pid_);
            int32_t errCode = ActiveBuffers();
            state_ = curState;
            CHECK_AND_RETURN_RET_LOG(errCode == AVCS_ERR_OK, errCode, "Active buffers failed!");
            return ret;
        }
        AVCODEC_LOGI("Buf[%{public}u] fd[%{public}u] swap out success!", i, fd);
        codecBuffer->hasSwapedOut = true;
    }
    return AVCS_ERR_OK;
}

int32_t RenderSurface::SwapInBuffers(bool isOutputBuffer) const
{
    uint32_t bufferType = isOutputBuffer ? INDEX_OUTPUT : INDEX_INPUT;
    CHECK_AND_RETURN_RET_LOGD(bufferType == INDEX_OUTPUT, AVCS_ERR_OK, "Input buffers can't be swapped in!");
    for (uint32_t i = 0u; i < buffers_[bufferType].size(); i++) {
        std::shared_ptr<CodecBuffer> codecBuffer = buffers_[bufferType][i];
        if (!codecBuffer->hasSwapedOut) {
            continue;
        }
        std::shared_ptr<FSurfaceMemory> surfaceMemory = codecBuffer->sMemory;
        CHECK_AND_CONTINUE_LOG(surfaceMemory != nullptr, "Buf[%{public}u]->sMemory error!", i);
        sptr<SurfaceBuffer> surfaceBuffer = surfaceMemory->GetSurfaceBuffer();
        CHECK_AND_CONTINUE_LOG(surfaceBuffer != nullptr, "Buf[%{public}u]->surfaceBuf error!", i);
        int32_t fd = surfaceBuffer->GetFileDescriptor();
        int32_t ret = DmaSwaper::GetInstance().SwapInDma(pid_, fd);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Buf[%{public}u] fd[%{public}u] swap in error!", i, fd);
        AVCODEC_LOGI("Buf[%{public}u] fd[%{public}u] swap in success!", i, fd);
        codecBuffer->hasSwapedOut = false;
    }
    return AVCS_ERR_OK;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS