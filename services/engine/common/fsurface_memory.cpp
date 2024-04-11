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
#include "fsurface_memory.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AvCodec-FSurfaceMemory"};
}
namespace OHOS {
namespace MediaAVCodec {
sptr<Surface> FSurfaceMemory::surface_ = nullptr;
BufferRequestConfig FSurfaceMemory::requestConfig_ = {0};
ScalingMode FSurfaceMemory::scalingMode_ = {ScalingMode::SCALING_MODE_SCALE_TO_WINDOW};


std::shared_ptr<FSurfaceMemory> FSurfaceMemory::Create()
{
    CHECK_AND_RETURN_RET_LOG(surface_ != nullptr, nullptr, "surface is nullptr");
    CHECK_AND_RETURN_RET_LOG(requestConfig_.width != 0 && requestConfig_.height != 0, nullptr,
                             "surface config invalid");
    std::shared_ptr<FSurfaceMemory> buffer = std::make_shared<FSurfaceMemory>();
    buffer->AllocSurfaceBuffer();
    return buffer;
}

FSurfaceMemory::~FSurfaceMemory()
{
    ReleaseSurfaceBuffer();
}

void FSurfaceMemory::AllocSurfaceBuffer()
{
    if (surface_ == nullptr || surfaceBuffer_ != nullptr) {
        AVCODEC_LOGE("surface is nullptr or surfaceBuffer is not nullptr");
        return;
    }
    fence_ = -1;
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    auto ret = surface_->RequestBuffer(surfaceBuffer, fence_, requestConfig_);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK || surfaceBuffer == nullptr) {
        if (ret == OHOS::SurfaceError::SURFACE_ERROR_NO_BUFFER) {
            AVCODEC_LOGD("buffer queue is no more buffers");
        } else {
            AVCODEC_LOGE("surface RequestBuffer fail, ret: %{public}" PRIu64, static_cast<uint64_t>(ret));
        }
        return;
    }

    surfaceBuffer_ = surfaceBuffer;
    AVCODEC_LOGD("request surface buffer success, releaseFence: %{public}d", fence_);
}

void FSurfaceMemory::ReleaseSurfaceBuffer()
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

sptr<SurfaceBuffer> FSurfaceMemory::GetSurfaceBuffer()
{
    if (!surfaceBuffer_) {
        // request surface buffer again when old buffer flush to nullptr
        AllocSurfaceBuffer();
    }
    return surfaceBuffer_;
}

int32_t FSurfaceMemory::GetSurfaceBufferStride()
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "surfaceBuffer is nullptr");
    auto bufferHandle = surfaceBuffer_->GetBufferHandle();
    if (bufferHandle == nullptr) {
        AVCODEC_LOGE("Fail to get bufferHandle");
        return AVCS_ERR_UNKNOWN;
    }
    stride_ = bufferHandle->stride;
    return stride_;
}

int32_t FSurfaceMemory::GetFence()
{
    return fence_;
}

void FSurfaceMemory::SetNeedRender(bool needRender)
{
    needRender_ = needRender;
}

void FSurfaceMemory::UpdateSurfaceBufferScaleMode()
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

void FSurfaceMemory::SetSurface(sptr<Surface> surface)
{
    surface_ = surface;
}

void FSurfaceMemory::SetConfig(int32_t width, int32_t height, int32_t format, uint64_t usage, int32_t strideAlign,
                               int32_t timeout)
{
    requestConfig_ = {.width = width,
                      .height = height,
                      .strideAlignment = strideAlign,
                      .format = format,
                      .usage = usage,
                      .timeout = timeout};
}

void FSurfaceMemory::SetScaleType(ScalingMode videoScaleMode)
{
    scalingMode_ = videoScaleMode;
}

uint8_t *FSurfaceMemory::GetBase() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, nullptr, "surfaceBuffer is nullptr");
    return static_cast<uint8_t *>(surfaceBuffer_->GetVirAddr());
}

int32_t FSurfaceMemory::GetSize() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, -1, "surfaceBuffer is nullptr");
    uint32_t size = surfaceBuffer_->GetSize();
    return static_cast<int32_t>(size);
}
} // namespace MediaAVCodec
} // namespace OHOS
