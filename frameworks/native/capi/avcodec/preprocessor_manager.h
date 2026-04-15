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

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "preprocessor.h"
#include "surface.h"

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

    void NotifyNewBufferAvailable();

private:
    struct SurfaceBufferWrapper {
        enum SurfaceType {
            PRODUCER,
            CONSUMER,
        } surfaceType;

        sptr<SyncFence> fence{nullptr};
        sptr<SurfaceBuffer> buffer{nullptr};
        wptr<Surface> surface{nullptr};
        int64_t timestamp{0};
        BufferFlushConfig flushConfig{};
        bool isProcessed{false};

        SurfaceBufferWrapper(wptr<Surface> argSurface, sptr<SurfaceBuffer> buf,
                             sptr<SyncFence> argFence, SurfaceType type)
            : surface(argSurface), buffer(buf), fence(argFence), surfaceType(type) {};
        ~SurfaceBufferWrapper();
    };

    class EncoderInfo : std::enable_shared_from_this {
    public:
        explicit EncoderInfo(std::string_view encoderId) : encoderId_(encoderId) {}
        ~EncoderInfo();
        void Init(std::shared_ptr<Preprocessor> preprocessor, sptr<Surface> surface);
        int32_t Start();
        void Stop();

        int32_t CheckAndAllocateSurfaceBufferFromEncSurface();
        void OnNewPendingBufferAvailable(std::shared_ptr<SurfaceBufferWrapper> buffer);
        void OnNewEncoderSurfaceBufferAvailable(uint32_t seq, const sptr<SyncFence>& fence);

    private:
        void SetBufferFlushConfig(std::shared_ptr<SurfaceBufferWrapper> &inputBuffer,
                                  std::shared_ptr<SurfaceBufferWrapper> &outputBuffer);
        void EncoderThreadLoop();

        std::queue<std::shared_ptr<SurfaceBufferWrapper>> pendingBuffers_{};
        std::map<uint32_t, sptr<SurfaceBuffer>> encoderSurfaceBufferPool;
        std::queue<std::shared_ptr<SurfaceBufferWrapper>> availableInputBuffers_{};
        std::shared_ptr<Preprocessor> preprocessor_{nullptr};
        sptr<Surface> encoderSurface_{nullptr};
        std::thread processThread_;
        bool isRunning_{false};
        std::mutex bufferQueueMutex_;
        std::condition_variable bufferQueueCv_;
        std::string_view encoderId_;
    };

    sptr<Surface> sharedConsumerSurface_{nullptr};
    sptr<Surface> sharedProducerSurface_{nullptr};
    std::map<std::string_view, EncoderInfo> encoders_{};
    std::mutex mutex_;
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_MANAGER_H