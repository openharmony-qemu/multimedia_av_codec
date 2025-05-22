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

int32_t FSurfaceMemory::AllocSurfaceBuffer()
{
    CHECK_AND_RETURN_RET_LOG(sInfo_->surface != nullptr, AVCS_ERR_UNKNOWN, "Surface is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!isAttached, AVCS_ERR_UNKNOWN, "Only support when not attach!");
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ == nullptr, AVCS_ERR_UNKNOWN, "Surface buffer is not nullptr!");
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer, AVCS_ERR_UNKNOWN, "Create surface buffer failed!");
    GSError err = surfaceBuffer->Alloc(sInfo_->requestConfig);
    CHECK_AND_RETURN_RET_LOG(err == GSERROR_OK, err, "Alloc surface buffer failed, GSERROR=%{public}d", err);
    SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    isAttached = false;
    AVCODEC_LOGI("Alloc surface buffer success seq=%{public}u", surfaceBuffer_->GetSeqNum());
    return AVCS_ERR_OK;
}

int32_t FSurfaceMemory::RequestSurfaceBuffer()
{
    CHECK_AND_RETURN_RET_LOG(sInfo_->surface != nullptr, AVCS_ERR_UNKNOWN, "Surface is nullptr");
    CHECK_AND_RETURN_RET_LOG(owner == Owner::OWNED_BY_SURFACE, AVCS_ERR_UNKNOWN, "Only support when owned by surface!");
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    auto ret = sInfo_->surface->RequestBuffer(surfaceBuffer, fence_, sInfo_->requestConfig);
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK || surfaceBuffer == nullptr) {
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_NO_BUFFER) {
            AVCODEC_LOGE("Request surface buffer fail, ret=%{public}" PRIu64, static_cast<uint64_t>(ret));
        }
        return ret;
    }
    SetSurfaceBuffer(surfaceBuffer, Owner::OWNED_BY_CODEC);
    return AVCS_ERR_OK;
}

void FSurfaceMemory::ReleaseSurfaceBuffer()
{
    surfaceBuffer_ = nullptr;
}

sptr<SurfaceBuffer> FSurfaceMemory::GetSurfaceBuffer()
{
    if (isAttached && owner == Owner::OWNED_BY_SURFACE) {
        CHECK_AND_RETURN_RET_LOG(RequestSurfaceBuffer() == AVCS_ERR_OK, nullptr, "Get surface buffer failed!");
    }
    return surfaceBuffer_;
}

void FSurfaceMemory::SetSurfaceBuffer(sptr<SurfaceBuffer> surfaceBuffer, Owner toChangeOwner)
{
    surfaceBuffer_ = surfaceBuffer;
    owner = toChangeOwner;
}

int32_t FSurfaceMemory::GetSurfaceBufferStride()
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, 0, "Surface buffer is nullptr!");
    auto bufferHandle = surfaceBuffer_->GetBufferHandle();
    CHECK_AND_RETURN_RET_LOG(bufferHandle != nullptr, AVCS_ERR_UNKNOWN, "Failed to get bufferHandle!");
    stride_ = bufferHandle->stride;
    return stride_;
}

sptr<SyncFence> FSurfaceMemory::GetFence()
{
    return fence_;
}

uint8_t *FSurfaceMemory::GetBase() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, nullptr, "Surface buffer is nullptr!");
    return static_cast<uint8_t *>(surfaceBuffer_->GetVirAddr());
}

int32_t FSurfaceMemory::GetSize() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, -1, "Surface buffer is nullptr!");
    uint32_t size = surfaceBuffer_->GetSize();
    return static_cast<int32_t>(size);
}
} // namespace MediaAVCodec
} // namespace OHOS
