/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef PREPROCESSOR_MANAGER_H
#define PREPROCESSOR_MANAGER_H

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "preprocessor.h"
#include "surface.h"
#include "sync_fence.h"

namespace OHOS {
namespace MediaAVCodec {

class PreprocessorManager : public std::enable_shared_from_this<PreprocessorManager> {
public:
    PreprocessorManager();
    ~PreprocessorManager();

    void RegisterEncoder(std::string_view encoderId, std::shared_ptr<Preprocessor> preprocessor,
                         sptr<Surface> producerSurface);
    void UnregisterEncoder(std::string_view encoderId);
    void SetEncoderRunning(std::string_view encoderId, bool isRunning);
    void CreateSharedSurface();
    sptr<Surface> GetSharedSurface();
    bool RequestNotifyEos(std::string_view encoderId, std::function<void()> notifyEosTask);
    void NotifyEosNowIfPending(std::string_view encoderId);

    void NotifyNewBufferAvailable();

    // === Buffer data query for GetInputDescription (reads from pendingBuffers_ directly) ===
    struct BufferDataInfo {
        int32_t width{0};
        int32_t height{0};
        int32_t format{0};   // GraphicPixelFormat from SurfaceBuffer::GetFormat()
        int32_t stride{0};
    };
    bool PeekPendingBufferData(std::string_view encoderId, BufferDataInfo& outInfo) const;

private:
    struct SurfaceBufferWrapper {
        enum SurfaceType {
            PRODUCER,
            CONSUMER,
        };

        wptr<Surface> surface{nullptr};
        sptr<SurfaceBuffer> buffer{nullptr};
        sptr<SyncFence> fence{nullptr};
        SurfaceType surfaceType;
        int64_t timestamp{0};
        BufferFlushConfig flushConfig{};
        bool isProcessed{false};

        SurfaceBufferWrapper() = default;
        SurfaceBufferWrapper(wptr<Surface> argSurface, sptr<SurfaceBuffer> buf,
                             sptr<SyncFence> argFence, SurfaceType type)
            : surface(argSurface), buffer(buf), fence(argFence), surfaceType(type) {}
        ~SurfaceBufferWrapper();
    };

    class EncoderInfo : public std::enable_shared_from_this<EncoderInfo> {
    public:
        explicit EncoderInfo(std::string encoderId) : encoderId_(std::move(encoderId)) {}
        ~EncoderInfo();
        void Init(std::shared_ptr<Preprocessor> preprocessor, sptr<Surface> surface);
        int32_t Start();
        void Stop();

        int32_t CheckAndAllocateSurfaceBufferFromEncSurface();
        void OnNewPendingBufferAvailable(std::shared_ptr<SurfaceBufferWrapper> buffer);
        void OnNewEncoderSurfaceBufferAvailable(uint32_t seq, const sptr<SyncFence>& fence);
        bool RequestNotifyEos(std::function<void()> notifyEosTask);
        void NotifyEosNowIfPending();

        // === Buffer data query for GetInputDescription (reads from pendingBuffers_ directly) ===
        bool PeekPendingBufferData(BufferDataInfo& outInfo) const;

    private:
        void SetBufferFlushConfig(std::shared_ptr<SurfaceBufferWrapper> &inputBuffer,
                                  std::shared_ptr<SurfaceBufferWrapper> &outputBuffer);
        bool DequeueBuffers(std::shared_ptr<SurfaceBufferWrapper> &inputBuffer,
                            std::shared_ptr<SurfaceBufferWrapper> &outputBuffer,
                            std::function<void()> &notifyEosTask);
        void ProcessFrame(std::shared_ptr<SurfaceBufferWrapper> &inputBuffer,
                          std::shared_ptr<SurfaceBufferWrapper> &outputBuffer);
        void EncoderThreadLoop();

        std::queue<std::shared_ptr<SurfaceBufferWrapper>> pendingBuffers_{};
        std::map<uint32_t, sptr<SurfaceBuffer>> encoderSurfaceBufferPool_;
        std::queue<std::shared_ptr<SurfaceBufferWrapper>> availableInputBuffers_{};
        std::shared_ptr<Preprocessor> preprocessor_{nullptr};
        sptr<Surface> encoderSurface_{nullptr};
        std::thread processThread_;
        bool isRunning_{false};
        bool eosRequested_{false};
        std::function<void()> notifyEosTask_{};
        mutable std::mutex bufferQueueMutex_;
        std::condition_variable bufferQueueCv_;
        std::string encoderId_;
    };

    sptr<Surface> sharedConsumerSurface_{nullptr};
    sptr<Surface> sharedProducerSurface_{nullptr};
    std::map<std::string, std::shared_ptr<EncoderInfo>> encoders_{};
    mutable std::mutex mutex_;
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_MANAGER_H
