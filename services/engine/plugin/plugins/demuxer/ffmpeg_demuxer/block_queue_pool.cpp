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

#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "block_queue_pool.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "BlockQueuePool"};
}

namespace OHOS {
namespace MediaAVCodec {
BlockQueuePool::~BlockQueuePool()
{
    AVCODEC_LOGD("block queue %{public}s ~BlockQueuePool enter.", name_.c_str());
    for (auto que : quePool_) {
        FreeQueue(que.first);
    }
    AVCODEC_LOGD("block queue %{public}s ~BlockQueuePool free finish.", name_.c_str());
}

int32_t BlockQueuePool::AddTrackQueue(uint32_t trackIndex)
{
    AVCODEC_LOGD("block queue %{public}s AddTrackQueue enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        uint32_t queIndex = GetValidQueue();
        queMap_[trackIndex] = std::vector<uint32_t>({ queIndex });
        AVCODEC_LOGD("block queue %{public}s AddTrackQueue finish, add track %{public}u, get queue %{public}u",
                     name_.c_str(), trackIndex, queIndex);
    } else {
        AVCODEC_LOGD("block queue %{public}s AddTrackQueue finish, track %{public}u is already in queue",
                     name_.c_str(), trackIndex);
    }
    return AVCS_ERR_OK;
}

int32_t BlockQueuePool::RemoveTrackQueue(uint32_t trackIndex)
{
    AVCODEC_LOGD("block queue %{public}s RemoveTrackQueue enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        AVCODEC_LOGD("block queue %{public}s RemoveTrackQueue finish, track %{public}u is not in queue",
                     name_.c_str(), trackIndex);
    } else {
        for (auto queIndex : queMap_[trackIndex]) {
            ResetQueue(queIndex);
        }
        queMap_[trackIndex].clear();
        queMap_.erase(trackIndex);
    }
    AVCODEC_LOGD("block queue %{public}s RemoveTrackQueue finish", name_.c_str());
    return AVCS_ERR_OK;
}

bool BlockQueuePool::HasCache(uint32_t trackIndex)
{
    AVCODEC_LOGD("block queue %{public}s HasCache enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    for (auto queIndex : queMap_[trackIndex]) {
        if (quePool_[queIndex].blockQue == nullptr) {
            AVCODEC_LOGD("block queue %{public}d is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            AVCODEC_LOGD("block queue %{public}s HasCache finish, result: have cache", name_.c_str());
            return true;
        }
    }
    AVCODEC_LOGD("block queue %{public}s HasCache finish, result: don't have cache", name_.c_str());
    return false;
}

void BlockQueuePool::ResetQueue(uint32_t queueIndex)
{
    auto blockQue = quePool_[queueIndex].blockQue;
    if (blockQue == nullptr) {
        return;
    }
    if (!blockQue->Empty()) {
        for (auto ele = blockQue->Pop(); ele != nullptr;) {
            av_packet_free(&(ele->pkt));
            ele = nullptr;
        }
    }
    blockQue->Clear();
    quePool_[queueIndex].isValid = true;
    return;
}

void BlockQueuePool::FreeQueue(uint32_t queueIndex)
{
    ResetQueue(queueIndex);
    quePool_[queueIndex].blockQue = nullptr;
}

bool BlockQueuePool::Push(uint32_t trackIndex, std::shared_ptr<SamplePacket> block)
{
    AVCODEC_LOGD("block queue %{public}s Push enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    int32_t ret;
    if (!HasQueue(trackIndex)) {
        AVCODEC_LOGW("trackIndex has not beed added, auto add first");
        ret = AddTrackQueue(trackIndex);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, false, "add new queue error: %{public}d", ret);
    }
    auto& queVector = queMap_[trackIndex];
    uint32_t pushIndex;
    if (queVector.size() > 0) {
        pushIndex = queVector[queVector.size() - 1];
    } else {
        pushIndex = GetValidQueue();
        queMap_[trackIndex].push_back(pushIndex);
        AVCODEC_LOGD("track has no queue, will get que %{public}d from pool", pushIndex);
    }
    if (InnerQueueIsFull(pushIndex)) {
        pushIndex = GetValidQueue();
        queMap_[trackIndex].push_back(pushIndex);
        AVCODEC_LOGD("track que is full, will get que %{public}d from pool", pushIndex);
    }
    if (quePool_[pushIndex].blockQue == nullptr) {
        AVCODEC_LOGD("block queue %{public}d is nullptr, failed to push data", pushIndex);
        return false;
    }
    return quePool_[pushIndex].blockQue->Push(block);
}

std::shared_ptr<SamplePacket> BlockQueuePool::Pop(uint32_t trackIndex)
{
    AVCODEC_LOGD("block queue %{public}s Pop enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        AVCODEC_LOGE("trackIndex: %{public}u has not cache queue", trackIndex);
        return nullptr;
    }
    auto& queVector = queMap_[trackIndex];
    for (auto index = 0; index < static_cast<int32_t>(queVector.size()); ++index) {
        auto queIndex = queVector[index];
        if (quePool_[queIndex].blockQue == nullptr) {
            AVCODEC_LOGD("block queue %{public}d is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            auto block = quePool_[queIndex].blockQue->Pop();
            if (quePool_[queIndex].blockQue->Empty()) {
                ResetQueue(queIndex);
                AVCODEC_LOGD("track %{public}u queue %{public}d is empty, will return to pool.", trackIndex, queIndex);
                queVector.erase(queVector.begin() + index);
            }
            AVCODEC_LOGD("block queue %{public}s Pop finish, trackIndex: %{public}u.", name_.c_str(), trackIndex);
            return block;
        }
    }
    AVCODEC_LOGE("trackIndex: %{public}u has not cache data", trackIndex);
    return nullptr;
}

std::shared_ptr<SamplePacket> BlockQueuePool::Front(uint32_t trackIndex)
{
    AVCODEC_LOGD("block queue %{public}s Pop enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    if (!HasQueue(trackIndex)) {
        AVCODEC_LOGE("trackIndex: %{public}u has not cache queue", trackIndex);
        return nullptr;
    }
    auto queVector = queMap_[trackIndex];
    for (int i = 0; i < static_cast<int32_t>(queVector.size()); ++i) {
        auto queIndex = queVector[i];
        if (quePool_[queIndex].blockQue == nullptr) {
            AVCODEC_LOGD("block queue %{public}d is nullptr, will find next", queIndex);
            continue;
        }
        if (quePool_[queIndex].blockQue->Size() > 0) {
            auto block = quePool_[queIndex].blockQue->Front();
            return block;
        }
    }
    AVCODEC_LOGE("trackIndex: %{public}u has not cache data", trackIndex);
    return nullptr;
}

uint32_t BlockQueuePool::GetValidQueue()
{
    AVCODEC_LOGD("block queue %{public}s GetValidQueue enter.", name_.c_str());
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
    AVCODEC_LOGD("block queue %{public}s GetValidQueue finish, valid queue index: %{public}u.",
                 name_.c_str(), queCount_);
    queCount_++;
    return (queCount_ - 1);
}

bool BlockQueuePool::InnerQueueIsFull(uint32_t queueIndex)
{
    AVCODEC_LOGD("block queue %{public}s InnerQueueIsFull enter, queueIndex: %{public}u.", name_.c_str(), queueIndex);
    if (quePool_[queueIndex].blockQue == nullptr) {
        AVCODEC_LOGD("block queue %{public}d is nullptr", queueIndex);
        return true;
    }
    return quePool_[queueIndex].blockQue->Size() >= quePool_[queueIndex].blockQue->Capacity();
}

bool BlockQueuePool::HasQueue(uint32_t trackIndex)
{
    AVCODEC_LOGD("block queue %{public}s HasQueue enter, trackIndex: %{public}u.", name_.c_str(), trackIndex);
    return queMap_.count(trackIndex) > 0;
}
} // namespace MediaAVCodec
} // namespace OHOS