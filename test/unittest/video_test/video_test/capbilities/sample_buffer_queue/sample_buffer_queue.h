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

#ifndef AVCODEC_SAMPLE_SAMPLE_BUFFER_QUEUE_H
#define AVCODEC_SAMPLE_SAMPLE_BUFFER_QUEUE_H

#include <optional>
#include <condition_variable>
#include <atomic>
#include "sample_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class SampleBufferQueue {
public:
    virtual int32_t QueueBuffer(const CodecBufferInfo& bufferInfo);
    virtual std::optional<CodecBufferInfo> DequeueBuffer();
    virtual int32_t Clear();
    virtual uint32_t GetFrameCount();
    virtual uint32_t IncFrameCount();

protected:
    std::atomic<uint32_t> frameCount_ = 0;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<CodecBufferInfo> bufferQueue_;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_SAMPLE_BUFFER_QUEUE_H