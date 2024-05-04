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


namespace OHOS {
namespace Media {

BlockQueuePool::~BlockQueuePool()
{
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " ~BlockQueuePool enter.", name_.c_str());
    for (auto que : quePool_) {
        FreeQueue(que.first);
    }
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " ~BlockQueuePool free finish.", name_.c_str());
}

Status BlockQueuePool::AddTrackQueue(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " AddTrackQueue enter, trackIndex: " PUBLIC_LOG_U32 ".",
        name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        uint32_t queIndex = GetValidQueue();
        queMap_[trackIndex] = std::vector<uint32_t>({ queIndex });
        MEDIA_LOG_D("block queue " PUBLIC_LOG_S " AddTrackQueue finish, add track " PUBLIC_LOG_U32
            ", get queue " PUBLIC_LOG_U32 "", name_.c_str(), trackIndex, queIndex);
        sizeMap_[trackIndex] = 0;
    } else {
        MEDIA_LOG_D("block queue " PUBLIC_LOG_S " AddTrackQueue finish, track " PUBLIC_LOG_U32 " is already in queue",
            name_.c_str(), trackIndex);
    }
    return Status::OK;
}

Status BlockQueuePool::RemoveTrackQueue(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " RemoveTrackQueue enter, trackIndex: " PUBLIC_LOG_U32 ".",
        name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_D("block queue " PUBLIC_LOG_S " RemoveTrackQueue finish, track " PUBLIC_LOG_U32 " is not in queue",
            name_.c_str(), trackIndex);
    } else {
        for (auto queIndex : queMap_[trackIndex]) {
            ResetQueue(queIndex);
        }
        queMap_[trackIndex].clear();
        queMap_.erase(trackIndex);
        sizeMap_.erase(trackIndex);
    }
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " RemoveTrackQueue finish", name_.c_str());
    return Status::OK;
}

bool BlockQueuePool::HasCache(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " HasCache enter, trackIndex: " PUBLIC_LOG_U32 ".",
        name_.c_str(), trackIndex);
    for (auto queIndex : queMap_[trackIndex]) {
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            MEDIA_LOG_D("block queue " PUBLIC_LOG_S " HasCache finish, result: have cache", name_.c_str());
            return true;
        }
    }
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " HasCache finish, result: don't have cache", name_.c_str());
    return false;
}

void BlockQueuePool::ResetQueue(uint32_t queueIndex)
{
    auto blockQue = quePool_[queueIndex].blockQue;
    if (blockQue == nullptr) {
        return;
    }
    blockQue->Clear();
    quePool_[queueIndex].isValid = true;
    return;
}

void BlockQueuePool::FreeQueue(uint32_t queueIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    ResetQueue(queueIndex);
    quePool_[queueIndex].blockQue = nullptr;
}

bool BlockQueuePool::Push(uint32_t trackIndex, std::shared_ptr<SamplePacket> block)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " Push enter, trackIndex: " PUBLIC_LOG_U32 ".", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_W("trackIndex has not beed added, auto add first");
        Status ret = AddTrackQueue(trackIndex);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, false, "add new queue error: " PUBLIC_LOG_D32 "", ret);
    }
    auto& queVector = queMap_[trackIndex];
    uint32_t pushIndex;
    if (queVector.size() > 0) {
        pushIndex = queVector[queVector.size() - 1];
    } else {
        pushIndex = GetValidQueue();
        queMap_[trackIndex].push_back(pushIndex);
        MEDIA_LOG_D("track has no queue, will get que " PUBLIC_LOG_D32 " from pool", pushIndex);
    }
    if (InnerQueueIsFull(pushIndex)) {
        pushIndex = GetValidQueue();
        queMap_[trackIndex].push_back(pushIndex);
        MEDIA_LOG_D("track que is full, will get que " PUBLIC_LOG_D32 " from pool", pushIndex);
    }
    if (quePool_[pushIndex].blockQue == nullptr) {
        MEDIA_LOG_D("block queue " PUBLIC_LOG_D32 " is nullptr, failed to push data", pushIndex);
        return false;
    }
    sizeMap_[trackIndex] += 1;
    return quePool_[pushIndex].blockQue->Push(block);
}

std::shared_ptr<SamplePacket> BlockQueuePool::Pop(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " Pop enter, trackIndex: " PUBLIC_LOG_U32 ".", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_E("trackIndex: " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return nullptr;
    }
    auto& queVector = queMap_[trackIndex];
    for (auto index = 0; index < static_cast<int32_t>(queVector.size()); ++index) {
        auto queIndex = queVector[index];
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            auto block = quePool_[queIndex].blockQue->Pop();
            if (quePool_[queIndex].blockQue->Empty()) {
                ResetQueue(queIndex);
                MEDIA_LOG_D("track " PUBLIC_LOG_U32 " queue " PUBLIC_LOG_D32 " is empty, will return to pool.",
                    trackIndex, queIndex);
                queVector.erase(queVector.begin() + index);
            }
            MEDIA_LOG_D("block queue " PUBLIC_LOG_S " Pop finish, trackIndex: " PUBLIC_LOG_U32 ".",
                name_.c_str(), trackIndex);
            if (sizeMap_[trackIndex] > 0) {
                sizeMap_[trackIndex] -= 1;
            }
            return block;
        }
    }
    MEDIA_LOG_E("trackIndex: " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

std::shared_ptr<SamplePacket> BlockQueuePool::Front(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " Front enter, trackIndex: " PUBLIC_LOG_U32 ".", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_E("trackIndex: " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
        return nullptr;
    }
    auto queVector = queMap_[trackIndex];
    for (int i = 0; i < static_cast<int32_t>(queVector.size()); ++i) {
        auto queIndex = queVector[i];
        if (quePool_[queIndex].blockQue == nullptr) {
            MEDIA_LOG_D("block queue " PUBLIC_LOG_D32 " is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            auto block = quePool_[queIndex].blockQue->Front();
            return block;
        }
    }
    MEDIA_LOG_E("trackIndex: " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

std::shared_ptr<SamplePacket> BlockQueuePool::Back(uint32_t trackIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " Back enter, trackIndex: " PUBLIC_LOG_U32 ".", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        MEDIA_LOG_E("trackIndex: " PUBLIC_LOG_U32 " has not cache queue", trackIndex);
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
    MEDIA_LOG_E("trackIndex: " PUBLIC_LOG_U32 " has not cache data", trackIndex);
    return nullptr;
}

uint32_t BlockQueuePool::GetValidQueue()
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " GetValidQueue enter.", name_.c_str());
    for (auto pair : quePool_) {
        if (pair.second.isValid && pair.second.blockQue != nullptr && pair.second.blockQue->Empty()) {
            quePool_[pair.first].isValid = false;
            return pair.first;
        }
    }
    quePool_[queCount_] = {
        false,
        std::make_shared<BlockQueue<std::shared_ptr<SamplePacket>>>("source_que_" + std::to_string(queCount_),
            singleQueSize_)
    };
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " GetValidQueue finish, valid queue index: " PUBLIC_LOG_U32 ".",
                 name_.c_str(), queCount_);
    queCount_++;
    return (queCount_ - 1);
}

bool BlockQueuePool::InnerQueueIsFull(uint32_t queueIndex)
{
    std::unique_lock<std::recursive_mutex> lockCacheQ(mutextCacheQ_);
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " InnerQueueIsFull enter, queueIndex: " PUBLIC_LOG_U32 ".",
        name_.c_str(), queueIndex);
    if (quePool_[queueIndex].blockQue == nullptr) {
        MEDIA_LOG_D("block queue " PUBLIC_LOG_D32 " is nullptr", queueIndex);
        return true;
    }
    return quePool_[queueIndex].blockQue->Size() >= quePool_[queueIndex].blockQue->Capacity();
}

bool BlockQueuePool::HasQueue(uint32_t trackIndex)
{
    MEDIA_LOG_D("block queue " PUBLIC_LOG_S " HasQueue enter, trackIndex: " PUBLIC_LOG_U32 ".",
        name_.c_str(), trackIndex);
    return queMap_.count(trackIndex) > 0;
}
} // namespace Media
} // namespace OHOS