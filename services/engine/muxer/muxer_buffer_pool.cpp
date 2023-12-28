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

#include "muxer_buffer_pool.h"
#include <limits>
#include "avcodec_log.h"
#include "avcodec_trace.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerBufferPool"};
}

namespace OHOS {
namespace MediaAVCodec {
MuxerBufferPool::MuxerBufferPool(std::string name, size_t capacity)
    : name_(std::move(name)), capacity_(capacity)
{
}

MuxerBufferPool::~MuxerBufferPool()
{
    Reset();
}

std::shared_ptr<uint8_t> MuxerBufferPool::AcquireBuffer(int32_t size)
{
    AVCodecTrace trace("MuxerBufferPool::AcquireBuffer");
    AVCODEC_LOGD("acquire buffer for size: %{public}d from pool %{public}s", size, name_.c_str());
    CHECK_AND_RETURN_RET_LOG(size > 0, nullptr, "acquire buffer size %{public}d is invalid", size);
    std::lock_guard<std::mutex> lock(mutex_);
    MuxerBuffer buffer {nullptr, 0};
    std::list<MuxerBuffer>::iterator minIdleSizeIter = idleList_.end();
    int32_t minIdleSize = std::numeric_limits<int32_t>::max();
    for (auto iter = idleList_.begin(); iter != idleList_.end(); ++iter) {
        if (iter->second >= size) {
            buffer.first = iter->first;
            buffer.second = iter->second;
            idleList_.erase(iter);
            break;
        }
        if (iter->second < minIdleSize) {
            minIdleSize = iter->second;
            minIdleSizeIter = iter;
        }
    }
    if (buffer.first == nullptr) {
        auto totalCount = busyList_.size() + idleList_.size();
        if (totalCount < capacity_) {
            buffer.first = std::shared_ptr<uint8_t>(new uint8_t[size], std::default_delete<uint8_t[]>());
            buffer.second = size;
        } else if (minIdleSizeIter != idleList_.end()) {
            idleList_.erase(minIdleSizeIter);
            buffer.first = std::shared_ptr<uint8_t>(new uint8_t[size], std::default_delete<uint8_t[]>());
            buffer.second = size;
        } else {
            AVCODEC_LOGD("the pool %{public}s is full, %{public}zu / %{public}zu",
                name_.c_str(), busyList_.size(), idleList_.size());
        }
    }
    if (buffer.first != nullptr) {
        busyList_.emplace_back(buffer);
    }
    return buffer.first;
}

void MuxerBufferPool::ReleaseBuffer(std::shared_ptr<uint8_t> buffer)
{
    AVCodecTrace trace("MuxerBufferPool::ReleaseBuffer");
    CHECK_AND_RETURN_LOG(buffer != nullptr, "release buffer is nullptr");
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto iter = busyList_.begin(); iter != busyList_.end(); ++iter) {
        if (iter->first.get() != buffer.get()) {
            continue;
        }
        idleList_.emplace_back(*iter);
        busyList_.erase(iter);
        AVCODEC_LOGD("the pool %{public}s release buffer, %{public}zu / %{public}zu",
            name_.c_str(), busyList_.size(), idleList_.size());
        break;
    }
}

void MuxerBufferPool::Reset()
{
    idleList_.clear();
    busyList_.clear();
}
}
} // namespace OHOS