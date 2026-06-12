/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SIDE_OUTPUT_SURFACE_PROCESSOR_H
#define SIDE_OUTPUT_SURFACE_PROCESSOR_H

#include "base_video_post_processor.h"
#include "video_post_processor_factory.h"
#include "surface.h"
#include "surface_buffer.h"
#include "sync_fence.h"
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace OHOS {
namespace Media {

class SideOutputSurfaceProcessor : public BaseVideoPostProcessor,
    public std::enable_shared_from_this<SideOutputSurfaceProcessor> {
public:
    SideOutputSurfaceProcessor();
    ~SideOutputSurfaceProcessor() override;

    Status Init() override;
    Status Flush() override;
    Status Stop() override;
    Status Start() override;
    Status Release() override;

    sptr<Surface> GetInputSurface() override;
    Status SetOutputSurface(sptr<Surface> surface) override;

    Status ReleaseOutputBuffer(uint32_t index, bool render) override;
    Status RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) override;

    Status SetCallback(const std::shared_ptr<PostProcessorCallback> callback) override;
    Status SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver) override;

    Status SetParameter(const Format &format) override;
    Status SetPostProcessorOn(bool isPostProcessorOn) override;
    Status SetVideoWindowSize(int32_t width, int32_t height) override;
    Status NotifyEos(int64_t eosPts = 0) override;

    void SetVideoOutput(sptr<Surface> sideOutputSurface) override;
    void OnConsumerBufferAvailable();
    void GetVideoSample(int32_t &result) override;
    void HandleSampleTask();
    void DealNoImage();

private:
    struct SurfaceBufferInfo {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int64_t timestamp;
    };

    GSError OnProducerBufferReleased();
    bool GetConsumerAndProducerBuffer(SurfaceBufferInfo& srcBufferInfo, SurfaceBufferInfo& dstBufferInfo);
    bool ProcessBuffer(SurfaceBufferInfo& srcBufferInfo, SurfaceBufferInfo& dstBufferInfo);
    void OutputBuffer(const SurfaceBufferInfo& bufferInfo);
    Status RenderOutputBufferInner(uint32_t index, int64_t renderTimestamp, bool render);
    bool FindBufferFromRenderQueue(uint32_t index, SurfaceBufferInfo& bufferInfo);
    Status FlushBufferToProducer(SurfaceBufferInfo& bufferInfo, int64_t renderTimestamp);
    void ProcessBuffers();
    bool WaitTrigger();
    void ClearBufferQueues();
    void PushBufferToConsumer(SurfaceBufferInfo& bufferInfo);
    void DealNotRenderBuffer(SurfaceBufferInfo& bufferInfo);
    bool CopyBufferToSideSurface(sptr<SurfaceBuffer> srcBuffer, int64_t timestamp);
    bool RequestSideBuffer(SurfaceBufferInfo& sideBufferInfo, sptr<Surface> sideSurface);
    bool FlushSideBuffer(SurfaceBufferInfo& sideBufferInfo, int64_t timestamp, sptr<Surface> sideSurface);

    bool UpdateRequestConfigFromBuffer(const SurfaceBufferInfo& bufferInfo);
    void HandleResolutionChange();
    bool HandleBufferSizeMismatch(SurfaceBufferInfo& srcBufferInfo, SurfaceBufferInfo& dstBufferInfo);
    sptr<SurfaceBuffer> HandleRenderBufferOverflow();
    void CancelOldBuffersOnResolutionChange(sptr<Surface> producerSurface);
    void UpdateConfigIfZero(sptr<SurfaceBuffer> srcBuffer);

    sptr<Surface> consumerSurface_;
    sptr<Surface> producerSurface_;
    sptr<Surface> sideSurface_;

    std::queue<SurfaceBufferInfo> consumerBufferQueue_;
    std::queue<SurfaceBufferInfo> producerBufferQueue_;
    std::map<uint32_t, SurfaceBufferInfo> renderBufferQueue_;
    sptr<SurfaceBuffer> pendingSideSrcBuffer_;
    int64_t pendingSideTimestamp_ {0};

    std::mutex consumerBufferLock_;
    std::mutex bufferLock_;
    std::mutex lock_;

    std::thread worker_;
    std::atomic<bool> isRunning_ {false};
    std::atomic<bool> isInitialized_ {false};
    std::atomic<bool> isProcessing_ {false};

    std::condition_variable cvTrigger_;
    std::condition_variable cvDone_;
    std::mutex waitLock_;

    std::atomic<bool> isBufferQueueReady_ {false};
    std::atomic<bool> needPrepareBuffers_ {false};

    BufferRequestConfig requestCfg_{};

    std::shared_ptr<PostProcessorCallback> callback_;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver_;

    enum class ProcessorState { IDLE, RUNNING, STOPPING };
    std::atomic<ProcessorState> state_ {ProcessorState::IDLE};

    std::shared_ptr<Task> sampleTask_;
    std::mutex sampleMutex_;
    std::condition_variable sampleDoneCond_;
    std::atomic<bool> sampleDone_ {false};
    std::atomic<bool> sampleProcessing_ {false};
    int32_t sampleResult_ {1};
    std::atomic<bool> isPaused_ {false};
};

} // namespace Media
} // namespace OHOS
#endif // SIDE_OUTPUT_SURFACE_PROCESSOR_H