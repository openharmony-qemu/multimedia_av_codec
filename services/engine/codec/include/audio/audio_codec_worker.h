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

#ifndef AV_CODEC_ADIO_CODEC_WORKER_H
#define AV_CODEC_ADIO_CODEC_WORKER_H

#include "audio_buffers_manager.h"
#include "audio_base_codec.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "nocopyable.h"
#include "task_thread.h"

#include <condition_variable>
#include <mutex>
#include <queue>

namespace OHOS {
namespace MediaAVCodec {
class AudioCodecWorker : public NoCopyable {
public:
    AudioCodecWorker(const std::shared_ptr<AudioBaseCodec> &codec, const std::shared_ptr<AVCodecCallback> &callback);

    ~AudioCodecWorker();

    bool PushInputData(const uint32_t &index);

    bool Configure();

    bool Start();

    bool Stop();

    bool Pause();

    bool Resume();

    bool Release();

    std::shared_ptr<AudioBuffersManager> GetInputBuffer() const noexcept;

    std::shared_ptr<AudioBuffersManager> GetOutputBuffer() const noexcept;

    std::shared_ptr<AudioBufferInfo> GetOutputBufferInfo(const uint32_t &index) const noexcept;

    std::shared_ptr<AudioBufferInfo> GetInputBufferInfo(const uint32_t &index) const noexcept;

private:
    void ProduceInputBuffer();
    void ConsumerOutputBuffer();
    void Dispose();
    bool Begin();
    bool HandInputBuffer(int32_t &ret);
    void ReleaseOutputBuffer(const uint32_t &index, const int32_t &ret);
    void SetFirstAndEosStatus(std::shared_ptr<AudioBufferInfo> &outBuffer, bool isEos, uint32_t index);
    void ReleaseAllInBufferQueue();
    void ReleaseAllInBufferAvaQueue();
    void ResetTask();

private:
    bool isFirFrame_;
    std::atomic<bool> isRunning;
    std::shared_ptr<AudioBaseCodec> codec_;
    int32_t inputBufferSize;
    int32_t outputBufferSize;
    const int16_t bufferCount;
    const std::string_view name_;
    std::mutex stateMutex_;
    std::mutex inAvaMutex_;
    std::mutex inputMutex_;
    std::mutex outputMutex_;
    std::condition_variable inputCondition_;
    std::condition_variable outputCondition_;

    std::unique_ptr<TaskThread> inputTask_;
    std::unique_ptr<TaskThread> outputTask_;
    std::shared_ptr<AVCodecCallback> callback_;
    std::shared_ptr<AudioBuffersManager> inputBuffer_;
    std::shared_ptr<AudioBuffersManager> outputBuffer_;
    std::queue<uint32_t> inBufIndexQue_;
    std::queue<uint32_t> inBufAvaIndexQue_;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif