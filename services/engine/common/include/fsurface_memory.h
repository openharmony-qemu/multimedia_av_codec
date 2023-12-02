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

class FSurfaceMemory {
public:
    FSurfaceMemory() = default;
    ~FSurfaceMemory();
    static std::shared_ptr<FSurfaceMemory> Create();
    static void SetSurface(sptr<Surface> surface);
    static void SetConfig(int32_t width, int32_t height, int32_t format, uint64_t usage = USAGE,
                          int32_t strideAlign = SURFACE_STRIDE_ALIGN, int32_t timeout = TIMEOUT);
    static void SetScaleType(ScalingMode videoScaleMode);
    void AllocSurfaceBuffer();
    void ReleaseSurfaceBuffer();
    sptr<SurfaceBuffer> GetSurfaceBuffer();
    int32_t GetSurfaceBufferStride();
    int32_t GetFence();
    void UpdateSurfaceBufferScaleMode();
    void SetNeedRender(bool needRender);
    uint8_t *GetBase() const;
    int32_t GetSize() const;

private:
    // Allocated memory size.
    sptr<SurfaceBuffer> surfaceBuffer_ = nullptr;
    int32_t fence_ = -1;
    int32_t stride_ = 0;
    bool needRender_ = false;
    static sptr<Surface> surface_;
    static BufferRequestConfig requestConfig_;
    static ScalingMode scalingMode_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif
