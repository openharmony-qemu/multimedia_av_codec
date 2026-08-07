/*
 * Copyright (C) 2023-2026 Huawei Device Co., Ltd.
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
#include "avpacket_wrapper.h"
#include "block_queue.h"
#include "common/log.h"
#include "common/status.h"

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr uint32_t BLOCK_QUEUE_LOG_LIMIT_FREQUENCY = 100;

namespace Ffmpeg {
struct SamplePacket {
    uint32_t offset = 0;
    std::vector<AVPacketWrapperPtr> pkts {};
    bool isEOS = false;
    bool isAnnexb = false;
    uint32_t queueIndex = 0;
    ~SamplePacket() = default;
};
} // namespace Ffmpeg

namespace MPEG4 {
struct Sample {
    enum SampleFlag : uint32_t {
        NONE = 0,
        EOS = 1 << 0,
        SYNC_FRAME = 1 << 1,
        DISCARD = 1 << 4,
    };
    int64_t pts;
    int64_t dts;
    int64_t duration;
    uint32_t flag;
    int32_t size;
    std::vector<uint8_t> skipSamplesInfo;
    std::unique_ptr<uint8_t[]> data;
};

struct MPEG4Sample {
    int32_t offset = 0;
    std::shared_ptr<Sample> sample = nullptr;
    bool isAnnexb = false;
    bool isVttc = false;
    uint32_t queueIndex = 0;
    uint32_t sampleIndex = 0;
};
} // namespace MPEG4

template<typename T>
struct BlockTraits {
    static uint32_t GetDataSize(const std::shared_ptr<T>& block);
    static void UpdateMaxPts(const std::shared_ptr<T>& block, int64_t& maxPts);
};

// 为SamplePacket特化
template<>
struct BlockTraits<Ffmpeg::SamplePacket> {
    static inline constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "BlockQueuePool" };
    static uint32_t GetDataSize(const std::shared_ptr<Ffmpeg::SamplePacket>& block)
    {
        uint32_t totalSize = 0;
        for (const auto &pktWrapper : block->pkts) {
            FALSE_CONTINUE_LOGD(pktWrapper != nullptr, "Pkt is nullptr, will find next");
            FALSE_CONTINUE_LOGD(pktWrapper->GetSize() > 0, "Invalid pkt size: " PUBLIC_LOG_D32, pktWrapper->GetSize());
            totalSize += static_cast<uint32_t>(pktWrapper->GetSize());
        }
        return totalSize;
    }

    static void UpdateMaxPts(const std::shared_ptr<Ffmpeg::SamplePacket>& block, int64_t& maxPts)
    {
        for (const auto &pktWrapper : block->pkts) {
            if (pktWrapper->GetPts() != AV_NOPTS_VALUE && pktWrapper->GetPts() > maxPts) {
                maxPts = pktWrapper->GetPts();
            }
        }
    }
};

// 为MPEG4Sample特化
template<>
struct BlockTraits<MPEG4::MPEG4Sample> {
    static uint32_t GetDataSize(const std::shared_ptr<MPEG4::MPEG4Sample>& block)
    {
        return block->sample != nullptr ? static_cast<uint32_t>(block->sample->size) : 0;
    }
    
    static void UpdateMaxPts(const std::shared_ptr<MPEG4::MPEG4Sample>& block, int64_t& maxPts)
    {
        if (block->sample != nullptr && block->sample->pts > maxPts) {
            maxPts = block->sample->pts;
        }
    }
};

template <typename T>
class BlockQueuePool {
public:
    static inline constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "BlockQueuePool" };

    explicit BlockQueuePool(std::string name, size_t singleQueSize = SINGLE_QUEUE_SIZE)
        : name_(std::move(name)), quePool_(), queMap_(), sizeMap_(), singleQueSize_(singleQueSize)
    {
    }
    ~BlockQueuePool();

    Status AddTrackQueue(uint32_t trackIndex);
    Status RemoveTrackQueue(uint32_t trackIndex);
    bool HasCache(uint32_t trackIndex);
    size_t GetCacheSize(uint32_t trackIndex);
    uint32_t GetCacheDataSize(uint32_t trackIndex);
    uint32_t GetCacheFrameCount(uint32_t trackIndex);
    bool ResetInfo(std::shared_ptr<T> block);
    bool SetInfo(std::shared_ptr<T> block);
    void FreeQueue(uint32_t queueIndex);
    bool Push(uint32_t trackIndex, std::shared_ptr<T> block);
    std::shared_ptr<T> Pop(uint32_t trackIndex);
    std::shared_ptr<T> Front(uint32_t trackIndex);
    std::shared_ptr<T> Back(uint32_t trackIndex);
    Status GetLastPTSByTrackId(uint32_t trackIndex, int64_t& maxPts);

private:
    struct InnerQueue {
        bool isValid {false};
        uint32_t dataSize {0};
        std::shared_ptr<BlockQueue<std::shared_ptr<T>>> blockQue {nullptr};
        int64_t maxPts {INT64_MIN};
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
    void ResetQueue(uint32_t queueIndex);
    std::recursive_mutex mutextCacheQ_ {};
};

template <typename T>
BlockQueuePool<T>::~BlockQueuePool()
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
    for (auto que : quePool_) {
        FreeQueue(que.first);
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
}

template <typename T>
Status BlockQueuePool<T>::AddTrackQueue(uint32_t trackIndex)
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

template <typename T>
Status BlockQueuePool<T>::RemoveTrackQueue(uint32_t trackIndex)
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

template <typename T>
size_t BlockQueuePool<T>::GetCacheSize(uint32_t trackIndex)
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

template <typename T>
uint32_t BlockQueuePool<T>::GetCacheDataSize(uint32_t trackIndex)
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

template <typename T>
uint32_t BlockQueuePool<T>::GetCacheFrameCount(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32,
        name_.c_str(), trackIndex);
    FALSE_RETURN_V_NOLOG(HasQueue(trackIndex), 0);
    auto it = sizeMap_.find(trackIndex);
    if (it == sizeMap_.end()) {
        MEDIA_LOG_D("Track " PUBLIC_LOG_U32 " not in sizeMap_", trackIndex);
        return 0;
    }
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32 " frame count = " PUBLIC_LOG_U32,
        name_.c_str(), trackIndex, it->second);
    return it->second;
}

template <typename T>
bool BlockQueuePool<T>::HasCache(uint32_t trackIndex)
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

template <typename T>
void BlockQueuePool<T>::ResetQueue(uint32_t queueIndex)
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

template <typename T>
bool BlockQueuePool<T>::ResetInfo(std::shared_ptr<T> block)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    FALSE_RETURN_V_MSG_E(block != nullptr, false, "Block is nullptr");
    MEDIA_LOG_D("Reset for block " PUBLIC_LOG_U32, block->queueIndex);
    uint32_t queIndex = block->queueIndex;
    FALSE_RETURN_V_MSG_E(quePool_.count(queIndex) > 0, false, "Index is invalid");
    if (quePool_[queIndex].blockQue == nullptr || quePool_[queIndex].blockQue->Size() == 0) {
        return true;
    }
    uint32_t blockSize = BlockTraits<T>::GetDataSize(block);
    quePool_[queIndex].dataSize = quePool_[queIndex].dataSize >= blockSize ?
        quePool_[queIndex].dataSize - blockSize : 0;
    return true;
}

template <typename T>
bool BlockQueuePool<T>::SetInfo(std::shared_ptr<T> block)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    FALSE_RETURN_V_MSG_E(block != nullptr, false, "Block is nullptr");
    MEDIA_LOG_D("Set for block " PUBLIC_LOG_U32, block->queueIndex);
    uint32_t queIndex = block->queueIndex;
    FALSE_RETURN_V_MSG_E(quePool_.count(queIndex) > 0, false, "Index is invalid");
    if (quePool_[queIndex].blockQue == nullptr || quePool_[queIndex].blockQue->Size() == 0) {
        return true;
    }
    uint32_t blockSize = BlockTraits<T>::GetDataSize(block);
    if (quePool_[queIndex].dataSize <= UINT32_MAX - blockSize) {
        quePool_[queIndex].dataSize += blockSize;
    } else {
        quePool_[queIndex].dataSize = UINT32_MAX;
    }
    return true;
}

template <typename T>
void BlockQueuePool<T>::FreeQueue(uint32_t queueIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    if (quePool_.count(queueIndex) == 0) {
        return;
    }
    ResetQueue(queueIndex);
    quePool_[queueIndex].blockQue = nullptr;
}

template <typename T>
bool BlockQueuePool<T>::Push(uint32_t trackIndex, std::shared_ptr<T> block)
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
    uint32_t blockSize = BlockTraits<T>::GetDataSize(block);
    if (quePool_[pushIndex].dataSize <= UINT32_MAX - blockSize) {
        quePool_[pushIndex].dataSize += blockSize;
    } else {
        quePool_[pushIndex].dataSize = UINT32_MAX;
    }
    BlockTraits<T>::UpdateMaxPts(block, quePool_[pushIndex].maxPts);
    block->queueIndex = pushIndex;
    return quePool_[pushIndex].blockQue->Push(block);
}

template <typename T>
std::shared_ptr<T> BlockQueuePool<T>::Pop(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
            "Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
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
        uint32_t blockSize = BlockTraits<T>::GetDataSize(block);
        quePool_[queIndex].dataSize = quePool_[queIndex].dataSize >= blockSize ?
            quePool_[queIndex].dataSize - blockSize : 0;
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
    MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
        "Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

template <typename T>
std::shared_ptr<T> BlockQueuePool<T>::Front(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
            "Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return nullptr;
    }
    auto queVector = queMap_[trackIndex];
    for (int32_t i = 0; i < static_cast<int32_t>(queVector.size()); ++i) {
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
    MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
        "Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

template <typename T>
std::shared_ptr<T> BlockQueuePool<T>::Back(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
            "Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
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
    MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
        "Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

template <typename T>
uint32_t BlockQueuePool<T>::GetValidQueue()
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
        std::make_shared<BlockQueue<std::shared_ptr<T>>>("source_que_" + std::to_string(queCount_),
            singleQueSize_)
    };
    MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S ", valid queue index: " PUBLIC_LOG_U32,
                 name_.c_str(), queCount_);
    queCount_++;
    return (queCount_ - 1);
}

template <typename T>
bool BlockQueuePool<T>::InnerQueueIsFull(uint32_t queueIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", queue " PUBLIC_LOG_U32, name_.c_str(), queueIndex);
    if (quePool_.count(queueIndex) <= 0 || quePool_[queueIndex].blockQue == nullptr) {
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_D32 " is nullptr", queueIndex);
        return true;
    }
    return quePool_[queueIndex].blockQue->Size() >= quePool_[queueIndex].blockQue->Capacity();
}

template <typename T>
bool BlockQueuePool<T>::HasQueue(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    return queMap_.count(trackIndex) > 0;
}

template <typename T>
Status BlockQueuePool<T>::GetLastPTSByTrackId(uint32_t trackIndex, int64_t& maxPts)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    maxPts = INT64_MIN;
    MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S ", track " PUBLIC_LOG_U32, name_.c_str(), trackIndex);
    if (!HasCache(trackIndex)) {
        MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
            "Track " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
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
    MEDIA_LOGW_LIMIT(BLOCK_QUEUE_LOG_LIMIT_FREQUENCY,
        "Track " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return Status::ERROR_NOT_EXISTED;
}

using Mpeg4BlockQueuePool = BlockQueuePool<MPEG4::MPEG4Sample>;
using FfmpegBlockQueuePool = BlockQueuePool<Ffmpeg::SamplePacket>;
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // BLOCK_QUEUE_POOL_H
