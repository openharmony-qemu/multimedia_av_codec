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
FSurfaceMemory::~FSurfaceMemory()
{
    ReleaseSurfaceBuffer();
}

void FSurfaceMemory::AllocSurfaceBuffer()
{
    CHECK_AND_RETURN_LOG(sInfo_->surface != nullptr, "surface info is nullptr");
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    auto ret = sInfo_->surface->RequestBuffer(surfaceBuffer, fence_, sInfo_->requestConfig);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK || surfaceBuffer == nullptr) {
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_NO_BUFFER) {
            AVCODEC_LOGE("surface RequestBuffer fail, ret: %{public}" PRIu64, static_cast<uint64_t>(ret));
        }
        return;
    }
    surfaceBuffer_ = surfaceBuffer;
}

void FSurfaceMemory::ReleaseSurfaceBuffer()
{
    CHECK_AND_RETURN_LOG(surfaceBuffer_ != nullptr, "surface buffer is nullptr");
    if (!needRender_) {
        auto ret = sInfo_->surface->CancelBuffer(surfaceBuffer_);
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            AVCODEC_LOGE("surface CancelBuffer fail, ret:  %{public}" PRIu64, static_cast<uint64_t>(ret));
        }
    }
    surfaceBuffer_ = nullptr;
}

sptr<SurfaceBuffer> FSurfaceMemory::GetSurfaceBuffer()
{
    if (!surfaceBuffer_) {
        AllocSurfaceBuffer();
    }
    return surfaceBuffer_;
}

int32_t FSurfaceMemory::GetSurfaceBufferStride()
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "surfaceBuffer is nullptr");
    auto bufferHandle = surfaceBuffer_->GetBufferHandle();
    CHECK_AND_RETURN_RET_LOG(bufferHandle != nullptr, AVCS_ERR_UNKNOWN, "Fail to get bufferHandle");
    stride_ = bufferHandle->stride;
    return stride_;
}

sptr<SyncFence> FSurfaceMemory::GetFence()
{
    return fence_;
}

void FSurfaceMemory::SetNeedRender(bool needRender)
{
    needRender_ = needRender;
}

void FSurfaceMemory::UpdateSurfaceBufferScaleMode()
{
    CHECK_AND_RETURN_LOG(surfaceBuffer_ != nullptr, "surface buffer is nullptr");
    auto ret = sInfo_->surface->SetScalingMode(surfaceBuffer_->GetSeqNum(), sInfo_->scalingMode);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        AVCODEC_LOGE("update surface buffer scaling mode fail, ret: %{public}" PRIu64, static_cast<uint64_t>(ret));
    }
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
