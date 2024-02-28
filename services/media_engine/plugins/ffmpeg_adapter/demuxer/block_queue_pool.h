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

#ifndef BLOCK_QUEUE_POOL_H
#define BLOCK_QUEUE_POOL_H
#include <vector>
#include <map>
#include <cstdint>
#include "block_queue.h"
#include "common/status.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace Media {

struct SamplePacket {
    uint32_t offset = 0;
    std::vector<AVPacket*> pkts {};
    bool isEOS = false;
    ~SamplePacket()
    {
        for (auto pkt : pkts) {
            if (pkt) {
                av_packet_free(&pkt);
            }
        }
    }
};

class BlockQueuePool {
public:
    explicit BlockQueuePool(std::string name, size_t singleQueSize = SINGLE_QUEUE_SIZE)
        : name_(std::move(name)), quePool_(), queMap_(), sizeMap_(), singleQueSize_(singleQueSize)
    {
    }
    ~BlockQueuePool();

    Status AddTrackQueue(uint32_t trackIndex);
    Status RemoveTrackQueue(uint32_t trackIndex);
    bool HasCache(uint32_t trackIndex);
    void ResetQueue(uint32_t queueIndex);
    void FreeQueue(uint32_t queueIndex);
    bool Push(uint32_t trackIndex, std::shared_ptr<SamplePacket> block);
    std::shared_ptr<SamplePacket> Pop(uint32_t trackIndex);
    std::shared_ptr<SamplePacket> Front(uint32_t trackIndex);
    std::shared_ptr<SamplePacket> Back(uint32_t trackIndex);
    
private:
    struct InnerQueue {
        bool isValid {false};
        std::shared_ptr<BlockQueue<std::shared_ptr<SamplePacket>>> blockQue {nullptr};
    };
    static constexpr size_t SINGLE_QUEUE_SIZE = 100;
    std::string name_;
    uint32_t queCount_ {0};
    std::map<uint32_t, InnerQueue> quePool_;
    std::map<uint32_t, std::vector<uint32_t>> queMap_;
    std::map<uint32_t, uint32_t> sizeMap_;
    size_t singleQueSize_ {0};

    uint32_t GetValidQueue();
    bool InnerQueueIsFull(uint32_t queueIndex);
    bool HasQueue(uint32_t trackIndex);
    std::recursive_mutex mutextCacheQ_ {};
};
} // namespace Media
} // namespace OHOS
#endif // BLOCK_QUEUE_POOL_H