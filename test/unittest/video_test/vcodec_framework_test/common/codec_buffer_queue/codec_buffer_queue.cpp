/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "codec_buffer_queue.h"

namespace OHOS {
namespace MediaAVCodec {
void CodecBufferQueue::Enqueue(const std::shared_ptr<CodecBufferInfo> bufferInfo)
{
    std::unique_lock<std::mutex> lock(mutex_);
    bufferQueue_.emplace(bufferInfo);
    cond_.notify_all();
}

std::shared_ptr<CodecBufferInfo> CodecBufferQueue::Dequeue(int32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    (void)cond_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() { return !bufferQueue_.empty(); });
    if (bufferQueue_.empty()) {
        return nullptr;
    }
    std::shared_ptr<CodecBufferInfo> bufferInfo = bufferQueue_.front();
    bufferQueue_.pop();
    return bufferInfo;
}

void CodecBufferQueue::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!bufferQueue_.empty()) {
        std::shared_ptr<CodecBufferInfo> bufferInfo = bufferQueue_.front();
        bufferInfo->Release();
        bufferQueue_.pop();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS