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

#ifndef RENDER_SURFACE_H
#define RENDER_SURFACE_H

#include <atomic>
#include <shared_mutex>
#include <vector>
#include "avcodec_common.h"
#include "block_queue.h"
#include "codec_utils.h"
#include "dma_swap.h"
#include "task_thread.h"
#include "coderstate.h"
#include "media_description.h"
#include "fsurface_memory.h"
#include "task_thread.h"
#include "surface_tools.h"
#include "avbuffer.h"
#include "fsurface_memory.h"

constexpr int32_t BITS_PER_PIXEL_COMPONENT_8 = 8;
constexpr int32_t BITS_PER_PIXEL_COMPONENT_10 = 10;

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
class RenderSurface : public RefBase {
public:
    RenderSurface() {}
    ~RenderSurface() {}
    void UnRegisterListenerToSurface(const sptr<Surface> &surface);
    void StopRequestSurfaceBufferThread();
    int32_t SetSurfaceCfg();
    int32_t ClearSurfaceAndSetQueueSize(const sptr<Surface> &surface, int32_t bufferCnt);
    void StartRequestSurfaceBufferThread();
    int32_t Attach(sptr<SurfaceBuffer> surfaceBuffer);
    int32_t FlushSurfaceMemory(std::shared_ptr<FSurfaceMemory> &surfaceMemory, uint32_t index);
    int32_t ReplaceOutputSurfaceWhenRunning(sptr<Surface> newSurface);
    void CombineConsumerUsage();
    int32_t RegisterListenerToSurface(const sptr<Surface> &surface);
    int32_t FreezeBuffers(State curState);
    int32_t ActiveBuffers();
    struct CodecBuffer {
    public:
        CodecBuffer() = default;
        ~CodecBuffer() = default;
        std::shared_ptr<AVBuffer> avBuffer = nullptr;
        std::shared_ptr<FSurfaceMemory> sMemory = nullptr;
        std::atomic<Owner> owner_ = Owner::OWNED_BY_US;
        int32_t width = 0;
        int32_t height = 0;
        int32_t bitDepth = BITS_PER_PIXEL_COMPONENT_8;
        std::atomic<bool> hasSwapedOut = false;
    };
    int pid_ = -1;
    std::atomic<State> state_ = State::UNINITIALIZED;
    SurfaceControl sInfo_;
    Format format_;
    int32_t instanceId_ = -1;
    int32_t width_ = 0;
    int32_t height_ = 0;
    VideoPixelFormat outputPixelFmt_ = VideoPixelFormat::UNKNOWN;
    std::shared_ptr<BlockQueue<uint32_t>> renderAvailQue_;
    std::shared_ptr<BlockQueue<uint32_t>> requestSurfaceBufferQue_;
    std::shared_ptr<BlockQueue<uint32_t>> codecAvailQue_;
    std::vector<std::shared_ptr<CodecBuffer>> buffers_[2];
    std::mutex surfaceMutex_;
    std::vector<std::shared_ptr<AVBuffer>> outAVBuffer4Surface_;
    int32_t outputBufferCnt_ = 0;
    std::mutex outputMutex_;
    std::map<uint32_t, std::pair<sptr<SurfaceBuffer>, OHOS::BufferFlushConfig>> renderSurfaceBufferMap_;
    std::atomic<GraphicTransformType> transform_ = GraphicTransformType::GRAPHIC_ROTATE_NONE;

private:
    int32_t SetQueueSize(const sptr<Surface> &surface, uint32_t targetSize);
    int32_t SwitchBetweenSurface(const sptr<Surface> &newSurface);
    bool RequestSurfaceBufferOnce(uint32_t index);
    int32_t RenderNewSurfaceWithOldBuffer(const sptr<Surface> &newSurface, uint32_t index);
    GSError BufferReleasedByConsumer(uint64_t surfaceId);
    void RequestBufferFromConsumer();
    void FindAvailIndex(uint32_t index) const;
    void RequestSurfaceBufferThread();
    bool CanSwapOut(bool isOutputBuffer, const std::shared_ptr<CodecBuffer> &codecBuffer);
    int32_t SwapOutBuffers(bool isOutputBuffer, State curState);
    int32_t SwapInBuffers(bool isOutputBuffer) const;
    sptr<SurfaceBuffer> CreateNewSurfaceBuffer(int32_t index);

    std::thread mRequestSurfaceBufferThread_;
    std::atomic<bool> requestSucceed_ = false;
    std::mutex requestBufferMutex_;
    std::mutex renderBufferMapMutex_;
    std::atomic<bool> requestBufferThreadExit_ = false;
    std::atomic<bool> requestBufferFinished_ = true;
    std::condition_variable requestBufferCV_;
    std::condition_variable requestBufferOnceDoneCV_;
};

} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // RENDER_SURFACE_H