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
constexpr uint64_t USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
constexpr int32_t SURFACE_STRIDE_ALIGN = 8;
constexpr int32_t TIMEOUT = 0;
} // namespace

struct SurfaceControl {
    sptr<Surface> surface = nullptr;
    BufferRequestConfig requestConfig = {.width = 0,
                                         .height = 0,
                                         .strideAlignment = SURFACE_STRIDE_ALIGN,
                                         .format = 0,
                                         .usage = USAGE,
                                         .timeout = TIMEOUT};
    ScalingMode scalingMode = ScalingMode::SCALING_MODE_SCALE_TO_WINDOW;
};

class FSurfaceMemory {
public:
    FSurfaceMemory(SurfaceControl *sInfo) : sInfo_(sInfo)
    {
        AllocSurfaceBuffer();
    }
    ~FSurfaceMemory();
    void AllocSurfaceBuffer();
    void ReleaseSurfaceBuffer();
    sptr<SurfaceBuffer> GetSurfaceBuffer();
    int32_t GetSurfaceBufferStride();
    sptr<SyncFence> GetFence();
    void UpdateSurfaceBufferScaleMode();
    void SetNeedRender(bool needRender);
    uint8_t *GetBase() const;
    int32_t GetSize() const;

private:
    sptr<SurfaceBuffer> surfaceBuffer_ = nullptr;
    sptr<SyncFence> fence_ = nullptr;
    int32_t stride_ = 0;
    bool needRender_ = false;
    SurfaceControl *sInfo_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif
