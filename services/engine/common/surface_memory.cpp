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

#include <memory>
#include "securec.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "surface_memory.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-SurfaceMemory"};
}
namespace OHOS {
namespace MediaAVCodec {
sptr<Surface> SurfaceMemory::surface_ = nullptr;
BufferRequestConfig SurfaceMemory::requestConfig_ = {0};
ScalingMode SurfaceMemory::scalingMode_ = {ScalingMode::SCALING_MODE_SCALE_TO_WINDOW};

std::shared_ptr<SurfaceMemory> SurfaceMemory::Create()
{
    CHECK_AND_RETURN_RET_LOG(surface_ != nullptr, nullptr, "surface is nullptr");
    CHECK_AND_RETURN_RET_LOG(requestConfig_.width != 0 && requestConfig_.height != 0, nullptr,
                             "surface config invalid");
    std::shared_ptr<SurfaceMemory> buffer = std::make_shared<SurfaceMemory>();
    (void)buffer->AllocSurfaceBuffer();
    return buffer;
}

SurfaceMemory::~SurfaceMemory()
{
    ReleaseSurfaceBuffer();
}

int32_t SurfaceMemory::Write(const uint8_t *in, int32_t writeSize, int32_t position)
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "surfaceBuffer is nullptr");
    int32_t start = 0;
    int32_t capacity = GetSize();
    if (position == INVALID_POSITION) {
        start = size_;
    } else {
        start = std::min(position, capacity);
    }
    int32_t length = std::min(writeSize, capacity - start);
    if (memcpy_s(GetBase() + start, length, in, length) != EOK) {
        return 0;
    }
    size_ = start + length;
    return length;
}

int32_t SurfaceMemory::Read(uint8_t *out, int32_t readSize, int32_t position)
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "surfaceBuffer is nullptr");
    int32_t start = 0;
    int32_t maxLength = size_;
    if (position != INVALID_POSITION) {
        start = std::min(position, size_);
        maxLength = size_ - start;
    }
    int32_t length = std::min(readSize, maxLength);
    if (memcpy_s(out, length, GetBase() + start, length) != EOK) {
        return 0;
    }
    return length;
}

int32_t SurfaceMemory::AllocSurfaceBuffer()
{
    CHECK_AND_RETURN_RET_LOG(surface_ != nullptr, AVCS_ERR_UNKNOWN,
                             "Allocate surface buffer failed: surface is nullptr");
    if (surfaceBuffer_ != nullptr) {
        AVCODEC_LOGI("surfaceBuffer is not nullptr");
        return AVCS_ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(requestConfig_.width != 0 && requestConfig_.height != 0, AVCS_ERR_UNKNOWN,
                             "surface config invalid");
    int32_t releaseFence = -1;
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    auto ret = surface_->RequestBuffer(surfaceBuffer, releaseFence, requestConfig_);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK || surfaceBuffer == nullptr) {
        if (ret == OHOS::SurfaceError::SURFACE_ERROR_NO_BUFFER) {
            AVCODEC_LOGD("buffer queue is no more buffers");
        } else {
            AVCODEC_LOGE("surface RequestBuffer fail, ret: %{public}" PRIu64, static_cast<uint64_t>(ret));
        }
        return AVCS_ERR_NO_MEMORY;
    }
    if (surfaceBuffer->Map() != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("surface buffer Map failed");
        surface_->CancelBuffer(surfaceBuffer);
        return AVCS_ERR_UNKNOWN;
    }
    sptr<SyncFence> autoFence = new (std::nothrow) SyncFence(releaseFence);
    if (autoFence != nullptr) {
        autoFence->Wait(100); // 100ms
    }
    ret = surface_->SetScalingMode(surfaceBuffer->GetSeqNum(), scalingMode_);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("Fail to set surface buffer scaling mode");
        surface_->CancelBuffer(surfaceBuffer);
        return AVCS_ERR_UNKNOWN;
    }

    auto bufferHandle = surfaceBuffer->GetBufferHandle();
    if (bufferHandle == nullptr) {
        AVCODEC_LOGE("Fail to get bufferHandle");
        return AVCS_ERR_UNKNOWN;
    }

    stride_ = bufferHandle->stride;
    surfaceBuffer_ = surfaceBuffer;
    fence_ = -1;
    AVCODEC_LOGD("request surface buffer success, releaseFence: %{public}d", releaseFence);
    return AVCS_ERR_OK;
}

void SurfaceMemory::ReleaseSurfaceBuffer()
{
    if (surfaceBuffer_ == nullptr) {
        return;
    }
    if (!needRender_) {
        auto ret = surface_->CancelBuffer(surfaceBuffer_);
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            AVCODEC_LOGE("surface CancelBuffer fail, ret:  %{public}" PRIu64, static_cast<uint64_t>(ret));
        }
    }
    surfaceBuffer_ = nullptr;
}

sptr<SurfaceBuffer> SurfaceMemory::GetSurfaceBuffer()
{
    return surfaceBuffer_;
}

int32_t SurfaceMemory::GetSurfaceBufferStride()
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "surfaceBuffer is nullptr");
    return stride_;
}

int32_t SurfaceMemory::GetFlushFence()
{
    return fence_;
}

void SurfaceMemory::ClearUsedSize()
{
    size_ = 0;
}

void SurfaceMemory::SetNeedRender(bool needRender)
{
    needRender_ = needRender;
}

void SurfaceMemory::UpdateSurfaceBufferScaleMode()
{
    if (surfaceBuffer_ == nullptr) {
        AVCODEC_LOGE("surfaceBuffer is nullptr");
        return;
    }
    auto ret = surface_->SetScalingMode(surfaceBuffer_->GetSeqNum(), scalingMode_);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("update surface buffer scaling mode fail, ret: %{public}" PRIu64, static_cast<uint64_t>(ret));
    }
}

void SurfaceMemory::SetSurface(sptr<Surface> surface)
{
    surface_ = surface;
}

void SurfaceMemory::SetConfig(int32_t width, int32_t height, int32_t format, uint64_t usage, int32_t strideAlign,
                              int32_t timeout)
{
    requestConfig_ = {.width = width,
                      .height = height,
                      .strideAlignment = strideAlign,
                      .format = format,
                      .usage = usage,
                      .timeout = timeout};
}

void SurfaceMemory::SetScaleType(ScalingMode videoScaleMode)
{
    scalingMode_ = videoScaleMode;
}

uint8_t *SurfaceMemory::GetBase() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, nullptr, "surfaceBuffer is nullptr");
    return static_cast<uint8_t *>(surfaceBuffer_->GetVirAddr());
}

int32_t SurfaceMemory::GetUsedSize() const
{
    return size_;
}

int32_t SurfaceMemory::GetSize() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, -1, "surfaceBuffer is nullptr");
    uint32_t size = surfaceBuffer_->GetSize();
    return static_cast<int32_t>(size);
}

uint32_t SurfaceMemory::GetFlags() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "surfaceBuffer is nullptr");
    return FLAGS_READ_WRITE;
}
} // namespace MediaAVCodec
} // namespace OHOS
