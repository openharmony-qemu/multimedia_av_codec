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

#ifndef CODEC_BUFFER_CIRCULAR_H
#define CODEC_BUFFER_CIRCULAR_H

#include <queue>
#include <shared_mutex>
#include <string>
#include "avcodec_common.h"
#include "avcodec_dfx_component.h"

namespace OHOS {
namespace MediaAVCodec {
class BufferConverter;
class CodecBufferCircular : public AVCodecDfxComponent {
public:
    CodecBufferCircular() = default;
    ~CodecBufferCircular();

    // Configure circular
    void SetConverter(std::shared_ptr<BufferConverter> &converter);
    int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback);
    int32_t SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback);

    void SetIsRunning(bool isRunning);
    bool CanEnableSyncMode();
    bool CanEnableAsyncMode();
    void EnableSyncMode();
    void EnableAsyncMode();
    bool IsSyncMode();
    void ResetFlag();

    // Caches
    void ClearCaches();
    void FlushCaches();

    // Sycn mode interface
    int32_t QueryInputBuffer(uint32_t &index, int64_t timeoutUs);
    int32_t QueryOutputBuffer(uint32_t &index, int64_t timeoutUs);
    std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index);
    std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index);

    // Handle buffer before send to proxy
    int32_t HandleInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    int32_t HandleInputBuffer(uint32_t index);
    int32_t HandleOutputBuffer(uint32_t index);
    void QueueInputBufferDone(uint32_t index);
    void ReleaseOutputBufferDone(uint32_t index);

    // Callback
    void OnError(AVCodecErrorType errorType, int32_t errorCode);
    void OnOutputFormatChanged(const Format &format);
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);

private:
    typedef enum : uint8_t {
        FLAG_NONE = 0,
        FLAG_IS_RUNNING = 1 << 0,
        FLAG_IS_SYNC = 1 << 1,
        FLAG_SYNC_ASYNC_CONFIGURED = 1 << 2,
        FLAG_STREAM_CHANGED = 1 << 3,
        FLAG_ERROR = 1 << 4,
        FLAG_INPUT_EOS = 1 << 5,
        FLAG_OUTPUT_EOS = 1 << 6,
    } CodecCircularFlag;
    typedef enum : uint8_t {
        OWNED_BY_SERVER = 0,
        OWNED_BY_CLIENT = 1,
        OWNED_BY_USER = 2,
    } BufferOwner;
    typedef struct BufferItem {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        std::shared_ptr<AVSharedMemory> memory = nullptr;
        std::shared_ptr<Format> parameter = nullptr;
        std::shared_ptr<Format> attribute = nullptr;
        BufferOwner owner = OWNED_BY_SERVER;
    } BufferItem;
    using IndexQueue = std::queue<uint32_t>;
    using BufferCache = std::unordered_map<uint32_t, BufferItem>;
    using BufferCacheIter = BufferCache::iterator;

    // Common
    static std::shared_ptr<Format> GetParameter(BufferCacheIter &iter);
    static std::shared_ptr<Format> GetAttribute(BufferCacheIter &iter);
    static const std::string &OwnerToString(BufferOwner owner);
    void PrintCaches(bool isOutput);

    BufferCache inCache_;
    BufferCache outCache_;
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::atomic<uint8_t> flag_ = FLAG_NONE;

    // Async mode
    void AsyncOnError(AVCodecErrorType errorType, int32_t errorCode);
    void AsyncOnOutputFormatChanged(const Format &format);
    void AsyncOnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer);
    void AsyncOnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer);
    void ConvertToSharedMemory(const std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory);

    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
    std::shared_ptr<MediaCodecCallback> mediaCb_ = nullptr;
    std::shared_ptr<MediaCodecParameterCallback> paramCb_ = nullptr;
    std::shared_ptr<MediaCodecParameterWithAttrCallback> attrCb_ = nullptr;
    std::shared_ptr<BufferConverter> converter_ = nullptr;

    // Sync mode
    void SyncOnError(AVCodecErrorType errorType, int32_t errorCode);
    void SyncOnOutputFormatChanged(const Format &format);
    void SyncOnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer);
    void SyncOnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer);
    int32_t QueryInputIndex(uint32_t &index, int64_t timeoutUs);
    int32_t QueryOutputIndex(uint32_t &index, int64_t timeoutUs);
    bool WaitForInputBuffer(std::unique_lock<std::mutex> &lock, int64_t timeoutUs);
    bool WaitForOutputBuffer(std::unique_lock<std::mutex> &lock, int64_t timeoutUs);

    std::condition_variable inCond_;
    std::condition_variable outCond_;
    IndexQueue inQueue_;
    IndexQueue outQueue_;
    int32_t lastError_ = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif