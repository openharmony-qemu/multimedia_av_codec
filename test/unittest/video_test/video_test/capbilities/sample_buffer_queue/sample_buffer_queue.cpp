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

#include "sample_buffer_queue.h"
#include <chrono>
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t SampleBufferQueue::QueueBuffer(const CodecBufferInfo& bufferInfo)
{
    std::unique_lock<std::mutex> lock(mutex_);
    bufferQueue_.emplace(bufferInfo);
    cond_.notify_all();
    return AVCODEC_SAMPLE_ERR_OK;
}

std::optional<CodecBufferInfo> SampleBufferQueue::DequeueBuffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    using namespace std::chrono_literals;

    (void)cond_.wait_for(lock, 5s, [this]() { return !bufferQueue_.empty(); });
    CHECK_AND_RETURN_RET(!bufferQueue_.empty(), std::nullopt);

    CodecBufferInfo bufferInfo = bufferQueue_.front();
    bufferQueue_.pop();
    frameCount_++;

    return bufferInfo;
}

int32_t SampleBufferQueue::Clear()
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto emptyQueue = std::queue<CodecBufferInfo>();
    bufferQueue_.swap(emptyQueue);

    return AVCODEC_SAMPLE_ERR_OK;
}

uint32_t SampleBufferQueue::GetFrameCount()
{
    return frameCount_;
}

uint32_t SampleBufferQueue::IncFrameCount()
{
    return ++frameCount_;
}
} // Sample
} // MediaAVCodec
} // OHOS