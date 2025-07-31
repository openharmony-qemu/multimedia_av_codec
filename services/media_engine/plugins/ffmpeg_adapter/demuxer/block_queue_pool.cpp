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

#define HST_LOG_TAG "BlockQueuePool"

#include "common/log.h"
#include "block_queue_pool.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "BlockQueuePool" };
}

namespace OHOS {
namespace Media {

BlockQueuePool::~BlockQueuePool()
{
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
    for (auto que : quePool_) {
        FreeQueue(que.first);
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
}

Status BlockQueuePool::AddTrackQueue(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        uint32_t queIndex = GetValidQueue();
        queMap_[trackIndex] = std::vector<uint32_t>({ queIndex });
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", add track " PUBLIC_LOG_U32
            ", get queue " PUBLIC_LOG_U32, name_.c_str(), trackIndex, queIndex);
        sizeMap_[trackIndex] = 0;
    } else {
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32 " is already in queue",
            name_.c_str(), trackIndex);
    }
    return Status::OK;
}

Status BlockQueuePool::RemoveTrackQueue(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_D("Block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32 " not in queue", name_.c_str(), trackIndex);
    } else {
        for (auto queIndex : queMap_[trackIndex]) {
            ResetQueue(queIndex);
        }
        queMap_[trackIndex].clear();
        queMap_.erase(trackIndex);
        sizeMap_.erase(trackIndex);
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
    return Status::OK;
}

size_t BlockQueuePool::GetCacheSize(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    size_t size = 0;
    FALSE_RETURN_V_NOLOG(HasQueue(trackIndex), size);
    for (auto queIndex : queMap_[trackIndex]) {
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " has cache", name_.c_str());
            size += quePool_[queIndex].blockQue->Size();
        }
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", size = " PUBLIC_LOG_ZU, name_.c_str(), size);
    return size;
}

uint32_t BlockQueuePool::GetCacheDataSize(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    uint32_t dataSize = 0;
    FALSE_RETURN_V_NOLOG(HasQueue(trackIndex), dataSize);
    for (auto queIndex : queMap_[trackIndex]) {
        if (static_cast<uint64_t>(dataSize) + static_cast<uint64_t>(quePool_[queIndex].dataSize) > UINT32_MAX) {
            MEDIA_LOG_D("DataSize(" PUBLIC_LOG_U64 ") is out of uint32",
                static_cast<uint64_t>(dataSize) + static_cast<uint64_t>(quePool_[queIndex].dataSize));
            return UINT32_MAX;
        }
        dataSize += quePool_[queIndex].dataSize;
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32 " cache size = " PUBLIC_LOG_U32,
        name_.c_str(), trackIndex, dataSize);
    return dataSize;
}

bool BlockQueuePool::HasCache(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    FALSE_RETURN_V_NOLOG(HasQueue(trackIndex), false);
    for (auto queIndex : queMap_[trackIndex]) {
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " have cache", name_.c_str());
            return true;
        }
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S " don't have cache", name_.c_str());
    return false;
}

void BlockQueuePool::ResetQueue(uint32_t queueIndex)
{
    if (quePool_.count(queueIndex) == 0) {
        MEDIA_LOG_D("Error queueIndex");
        return;
    }
    auto blockQue = quePool_[queueIndex].blockQue;
    if (blockQue == nullptr) {
        return;
    }
    blockQue->Clear();
    quePool_[queueIndex].dataSize = 0;
    quePool_[queueIndex].isValid = true;
    quePool_[queueIndex].maxPts = INT64_MIN;
    return;
}

bool BlockQueuePool::ResetInfo(std::shared_ptr<SamplePacket> block)
{
    FALSE_RETURN_V_MSG_E(block != nullptr, false, "Block is nullptr");
    MEDIA_LOG_D("Reset for block " PUBLIC_LOG_U32, block->queueIndex);
    uint32_t queIndex = block->queueIndex;
    FALSE_RETURN_V_MSG_E(quePool_.count(queIndex) > 0, false, "Index is invalid");
    for (auto pkt : block->pkts) {
        if (pkt == nullptr) {
            MEDIA_LOG_D("Pkt is nullptr, will find next");
            continue;
        }
        uint32_t pktSize = static_cast<uint32_t>(pkt->size);
        if (quePool_[queIndex].dataSize >= pktSize) {
            quePool_[queIndex].dataSize -= pktSize;
        } else {
            quePool_[queIndex].dataSize = 0;
        }
    }
    return true;
}

bool BlockQueuePool::SetInfo(std::shared_ptr<SamplePacket> block)
{
    FALSE_RETURN_V_MSG_E(block != nullptr, false, "Block is nullptr");
    MEDIA_LOG_D("Set for block " PUBLIC_LOG_U32, block->queueIndex);
    uint32_t queIndex = block->queueIndex;
    FALSE_RETURN_V_MSG_E(quePool_.count(queIndex) > 0, false, "Index is invalid");
    for (auto pkt : block->pkts) {
        if (pkt == nullptr) {
            MEDIA_LOG_D("Pkt is nullptr, will find next");
            continue;
        }
        uint32_t pktSize = static_cast<uint32_t>(pkt->size);
        if (quePool_[queIndex].dataSize <= UINT32_MAX - pktSize) {
            quePool_[queIndex].dataSize += pktSize;
        } else {
            quePool_[queIndex].dataSize = UINT32_MAX;
        }
    }
    return true;
}

void BlockQueuePool::FreeQueue(uint32_t queueIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    if (quePool_.count(queueIndex) == 0) {
        return;
    }
    ResetQueue(queueIndex);
    quePool_[queueIndex].blockQue = nullptr;
}

bool BlockQueuePool::Push(uint32_t trackIndex, std::shared_ptr<SamplePacket> block)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        Status ret = AddTrackQueue(trackIndex);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "Add new queue error: " PUBLIC_LOG_D32, ret);
    }
    auto& queVector = queMap_[trackIndex];
    uint32_t pushIndex;
    if (queVector.size() > 0) {
        pushIndex = queVector[queVector.size() - 1];
    } else {
        pushIndex = GetValidQueue();
        queMap_[trackIndex].push_back(pushIndex);
        MEDIA_LOG_D("Track has no queue, will request " PUBLIC_LOG_D32 " from pool", pushIndex);
    }
    if (InnerQueueIsFull(pushIndex)) {
        pushIndex = GetValidQueue();
        queMap_[trackIndex].push_back(pushIndex);
        MEDIA_LOG_D("Track que is full, will request " PUBLIC_LOG_D32 " from pool", pushIndex);
    }
    if (quePool_[pushIndex].blockQue == nullptr) {
        MEDIA_LOG_D("Block queue " PUBLIC_LOG_D32 " is nullptr, failed to push data", pushIndex);
        return false;
    }
    sizeMap_[trackIndex] += 1;
    for (auto pkt : block->pkts) {
        if (pkt == nullptr) {
            continue;
        }
        quePool_[pushIndex].dataSize += static_cast<uint32_t>(pkt->size);
        if (pkt->pts != AV_NOPTS_VALUE && pkt->pts > quePool_[pushIndex].maxPts) {
            quePool_[pushIndex].maxPts = pkt->pts;
        }
    }
    block->queueIndex = pushIndex;
    return quePool_[pushIndex].blockQue->Push(block);
}

std::shared_ptr<SamplePacket> BlockQueuePool::Pop(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return nullptr;
    }
    auto& queVector = queMap_[trackIndex];
    for (auto it = queVector.begin(); it != queVector.end(); ++it) {
        int32_t index = std::distance(queVector.begin(), it);
        auto queIndex = queVector[index];
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() == 0) {
            continue;
        }
        auto block = quePool_[queIndex].blockQue->Pop();
        if (block == nullptr) {
            MEDIA_LOG_D("Block is nullptr");
            continue;
        }
        for (auto pkt : block->pkts) {
            if (pkt == nullptr) {
                MEDIA_LOG_D("Pkt is nullptr, will find next");
                continue;
            }
            uint32_t pktSize = static_cast<uint32_t>(pkt->size);
            quePool_[queIndex].dataSize =
                quePool_[queIndex].dataSize >= pktSize ? quePool_[queIndex].dataSize -= pktSize : 0;
        }
        if (quePool_[queIndex].blockQue->Empty()) {
            ResetQueue(queIndex);
            quePool_[queIndex].maxPts = INT64_MIN;
            MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " queue " PUBLIC_LOG_D32 " is empty, will return to pool",
                trackIndex, queIndex);
            queVector.erase(queVector.begin() + index);
        }
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S " for track " PUBLIC_LOG_U32,
            name_.c_str(), trackIndex);
        if (sizeMap_[trackIndex] > 0) {
            sizeMap_[trackIndex] -= 1;
        }
        return block;
    }
    MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

std::shared_ptr<SamplePacket> BlockQueuePool::Front(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return nullptr;
    }
    auto queVector = queMap_[trackIndex];
    for (int i = 0; i < static_cast<int32_t>(queVector.size()); ++i) {
        auto queIndex = queVector[i];
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            auto block = quePool_[queIndex].blockQue->Front();
            return block;
        }
    }
    MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

std::shared_ptr<SamplePacket> BlockQueuePool::Back(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return nullptr;
    }
    auto queVector = queMap_[trackIndex];
    if (queVector.size() > 0) {
        auto lastQueIndex = queVector[queVector.size() - 1];
        if (quePool_[lastQueIndex].blockQue != nullptr && quePool_[lastQueIndex].blockQue->Size() > 0) {
            auto block = quePool_[lastQueIndex].blockQue->Back();
            return block;
        }
    }
    MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

uint32_t BlockQueuePool::GetValidQueue()
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
    for (auto pair : quePool_) {
        if (pair.second.isValid && pair.second.blockQue != nullptr && pair.second.blockQue->Empty()) {
            quePool_[pair.first].isValid = false;
            return pair.first;
        }
    }
    quePool_[queCount_] = {
        false,
        0,
        std::make_shared<BlockQueue<std::shared_ptr<SamplePacket>>>("source_que_" + std::to_string(queCount_),
            singleQueSize_)
    };
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", valid queue index: " PUBLIC_LOG_U32,
                 name_.c_str(), queCount_);
    queCount_++;
    return (queCount_ - 1);
}

bool BlockQueuePool::InnerQueueIsFull(uint32_t queueIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", queue " PUBLIC_LOG_U32, name_.c_str(), queueIndex);
    if (quePool_.count(queueIndex) > 0 && quePool_[queueIndex].blockQue == nullptr) {
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_D32 " is nullptr", queueIndex);
        return true;
    }
    return quePool_[queueIndex].blockQue->Size() >= quePool_[queueIndex].blockQue->Capacity();
}

bool BlockQueuePool::HasQueue(uint32_t trackIndex)
{
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    return queMap_.count(trackIndex) > 0;
}

Status BlockQueuePool::GetLastPTSByTrackId(uint32_t trackIndex, int64_t& maxPts)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    maxPts = INT64_MIN;
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasCache(trackIndex)) {
        MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return Status::ERROR_NOT_EXISTED;
    }
    auto queVector = queMap_[trackIndex];
    for (auto queIndex : queVector) {
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_U32 " is nullptr, will find next", queIndex);
            continue;
        }
        maxPts = quePool_[queIndex].maxPts;
        return Status::OK;
    }
    MEDIA_LOG_E("Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return Status::ERROR_NOT_EXISTED;
}
} // namespace Media
} // namespace OHOS