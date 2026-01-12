/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AV_CODEC_FSURFACE_MEMORY_H
#define AV_CODEC_FSURFACE_MEMORY_H

#include "refbase.h"
#include "surface.h"
#include "sync_fence.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr uint64_t SURFACE_DEFAULT_USAGE =
    BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE;
constexpr int32_t SURFACE_STRIDE_ALIGN = 8;
constexpr int32_t TIMEOUT = 0;
} // namespace

enum class Owner {
    OWNED_BY_US,
    OWNED_BY_CODEC,
    OWNED_BY_USER,
    OWNED_BY_SURFACE,
};

struct CallerInfo {
    int32_t pid = -1;
    std::string instanceId = "";
    std::string processName = "";
    std::string_view mimeType = "";
    bool calledByAvcodec = true;
};

struct SurfaceBufferInfo {
    sptr<SurfaceBuffer> buf = nullptr;
    sptr<SyncFence> fence = nullptr;
    uint32_t seqNum = 0u;
};

struct SurfaceControl {
    sptr<Surface> surface = nullptr;
    BufferRequestConfig requestConfig = {.width = 0,
                                         .height = 0,
                                         .strideAlignment = SURFACE_STRIDE_ALIGN,
                                         .format = 0,
                                         .usage = SURFACE_DEFAULT_USAGE,
                                         .timeout = TIMEOUT};
    std::optional<ScalingMode> scalingMode = std::nullopt;
};

class FSurfaceMemory {
public:
    FSurfaceMemory(SurfaceControl *sInfo, CallerInfo &decInfo) : decInfo_(decInfo), sInfo_(sInfo)
    {
        isAttached = false;
    }
    ~FSurfaceMemory();
    int32_t AllocSurfaceBuffer(int32_t width, int32_t height);
    void ReleaseSurfaceBuffer();
    sptr<SurfaceBuffer> GetSurfaceBuffer();
    void SetSurfaceBuffer(sptr<SurfaceBuffer> surfaceBuffer, Owner toChangeOwner, sptr<SyncFence> fence = nullptr);
    int32_t GetSurfaceBufferStride();
    sptr<SyncFence> GetFence();
    uint8_t *GetBase() const;
    int32_t GetSize() const;
    uint32_t GetSurfaceBufferSeqNum() const;
    std::atomic<bool> isAttached = false;
    std::atomic<Owner> owner = Owner::OWNED_BY_US;

private:
    int32_t RequestSurfaceBuffer();
    void SetCallerToBuffer(int32_t w, int32_t h);
    CallerInfo decInfo_;
    sptr<SurfaceBuffer> surfaceBuffer_ = nullptr;
    sptr<SyncFence> fence_ = nullptr;
    int32_t stride_ = 0;
    SurfaceControl *sInfo_ = nullptr;
    uint32_t seqNum_ = 0u;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif
