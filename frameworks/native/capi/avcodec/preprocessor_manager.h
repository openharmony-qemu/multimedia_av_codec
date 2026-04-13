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
    void OnEncoderFrameDone(std::string_view encoderId, sptr<SurfaceBuffer> buffer);

    void CreateSharedSurface();
    int32_t Start();
    void Stop();

    sptr<Surface> GetSharedSurface();

private:
    void MainThreadLoop();
    void EncoderThreadLoop(std::string_view encoderId);
    void NotifyEncoders();
    void MarkAsSkipped(std::string_view encoderId);
    bool ShouldRelease(uint32_t bufferId);
    void CheckAndRelease(uint32_t bufferId);

    struct EncoderInfo {
        std::shared_ptr<Preprocessor> preprocessor{nullptr};
        sptr<Surface> encoderSurface{nullptr};
        bool isRunning{false};
        std::set<uint32_t> processedBuffers{};
    };

    sptr<Surface> sharedConsumerSurface_{nullptr};
    sptr<Surface> sharedProducerSurface_{nullptr};
    std::map<std::string_view, EncoderInfo> encoders_{};
    std::map<std::string_view, std::thread> encoderThreads_{};
    std::set<std::string_view> runningEncoders_{};

    struct BufferPending {
        sptr<SurfaceBuffer> buffer{nullptr};
        std::set<std::string_view> pendingEncoders{};
        std::set<std::string_view> processedEncoders{};
    };
    std::map<uint32_t, BufferPending> bufferPending_{};

    std::thread mainThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::set<uint32_t> releasedBuffers_{};
    bool isRunning_{false};
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // PREPROCESSOR_MANAGER_H