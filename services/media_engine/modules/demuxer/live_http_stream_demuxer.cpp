/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "LiveHttpStreamDemuxer"

#include "live_http_stream_demuxer.h"

#include <algorithm>
#include <map>
#include <memory>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "common/log.h"
#include "frame_detector.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "source/source.h"

namespace OHOS {
namespace Media {

const int32_t TRY_READ_SLEEP_TIME = 10;  //ms
const int32_t TRY_READ_TIMES = 10;
LiveHttpStreamDemuxer::LiveHttpStreamDemuxer()
{
    MEDIA_LOG_I("LiveHttpStreamDemuxer called");
}

LiveHttpStreamDemuxer::~LiveHttpStreamDemuxer()
{
    MEDIA_LOG_I("~LiveHttpStreamDemuxer called");
    ResetAllCache();
}

bool LiveHttpStreamDemuxer::GetPeekRangeSub(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    auto ret = PullData(streamID, offset, size, bufferPtr);
    if (ret == Status::ERROR_AGAIN) {
        isIgnoreRead_ = true;
        return true;
    } else {
        isIgnoreRead_ = false;
    }
    return Status::OK == ret;
}

bool LiveHttpStreamDemuxer::TryReadCache(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    if (cacheDataMap_.find(streamID) != cacheDataMap_.end()) {
        MEDIA_LOG_I("GetPeekRange read cache, offset: " PUBLIC_LOG_U64, offset);
        if (cacheDataMap_[streamID].CheckCacheExist(offset)) {
            auto memory = cacheDataMap_[streamID].GetData()->GetMemory();
            if (memory != nullptr && memory->GetSize() > 0) {
                MEDIA_LOG_D("GetPeekRange read cache, Read data from cache data.");
                return PullDataWithCache(streamID, offset, size, bufferPtr);
            }
        }
    }
    return false;
}

bool LiveHttpStreamDemuxer::GetPeekRange(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    if (bufferPtr == nullptr) {
        MEDIA_LOG_E("GetPeekRange bufferPtr invalid.");
        return false;
    }
    bool ret = false;
    bufferPtr->streamID = streamID;
    if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        if (IsDash()) {
            ret = TryReadCache(streamID, offset, size, bufferPtr);
            if (ret == true) {
                return ret;
            }
        }
        return GetPeekRangeSub(streamID, offset, size, bufferPtr);
    }
    ret = TryReadCache(streamID, offset, size, bufferPtr);
    if (ret == true) {
        return ret;
    }
    return PullDataWithoutCache(streamID, offset, size, bufferPtr);
}

Status LiveHttpStreamDemuxer::Init(const std::string& uri)
{
    MediaAVCodec::AVCodecTrace trace("VodStreamDemuxer::Init");
    MEDIA_LOG_I("VodStreamDemuxer::Init called");
    checkRange_ = [](int32_t streamID, uint64_t offset, uint32_t size) {
        return true;
    };
    peekRange_ = [this](int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        return GetPeekRange(streamID, offset, size, bufferPtr);
    };
    getRange_ = peekRange_;
    uri_ = uri;
    return Status::OK;
}

bool LiveHttpStreamDemuxer::PullDataWithCache(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    auto memory = cacheDataMap_[streamID].GetData()->GetMemory();

    MEDIA_LOG_D("PullDataWithCache, Read data from cache data.");
    uint64_t offsetInCache = offset - cacheDataMap_[streamID].GetOffset();
    if (size <= memory->GetSize() - offsetInCache) {
        bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, size, 0);
        return true;
    }
    bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, memory->GetSize() - offsetInCache, 0);
    uint64_t remainOffset = cacheDataMap_[streamID].GetOffset() + memory->GetSize();
    uint64_t remainSize = size - (memory->GetSize() - offsetInCache);
    std::shared_ptr<Buffer> tempBuffer = Buffer::CreateDefaultBuffer(remainSize);
    if (tempBuffer == nullptr || tempBuffer->GetMemory() == nullptr) {
        MEDIA_LOG_W("PullDataWithCache, Read data from cache data. only get partial data.");
        return true;
    }
    Status ret = PullData(streamID, remainOffset, remainSize, tempBuffer);
    if (ret == Status::OK) {
        bufferPtr->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize() - offsetInCache);
        if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
            MEDIA_LOG_W("PullDataWithCache, not cache begin.");
            return true;
        }
        std::shared_ptr<Buffer> mergedBuffer = Buffer::CreateDefaultBuffer(
            tempBuffer->GetMemory()->GetSize() + memory->GetSize());
        if (mergedBuffer == nullptr || mergedBuffer->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullDataWithCache, Read data from cache data success. update cache data fail.");
            return true;
        }
        mergedBuffer->GetMemory()->Write(memory->GetReadOnlyData(), memory->GetSize(), 0);
        mergedBuffer->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize());
        cacheDataMap_[streamID].SetData(mergedBuffer);
        MEDIA_LOG_I("PullDataWithCache, offset: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
            ", cache size: " PUBLIC_LOG_ZU, offset, cacheDataMap_[streamID].GetOffset(),
            cacheDataMap_[streamID].GetData()->GetMemory()->GetSize());
    }
    return ret == Status::OK;
}

bool LiveHttpStreamDemuxer::PullDataWithoutCache(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    Status ret = PullData(streamID, offset, size, bufferPtr);
    if (ret != Status::OK) {
        MEDIA_LOG_E("PullDataWithoutCache, PullData error " PUBLIC_LOG_D32, static_cast<int32_t>(ret));
        return false;
    }
    if (cacheDataMap_.find(streamID) != cacheDataMap_.end()) {
        MEDIA_LOG_I("PullDataWithoutCache, cacheDataMap_ exist streamID , do nothing.");
        if (IsDash()) {
            MEDIA_LOG_I("dash PullDataWithoutCache, cacheDataMap_ exist streamID , merge it.");
            auto memory = cacheDataMap_[streamID].GetData()->GetMemory();
            std::shared_ptr<Buffer> mergedBuffer = Buffer::CreateDefaultBuffer(
                bufferPtr->GetMemory()->GetSize() + memory->GetSize());
            if (mergedBuffer == nullptr || mergedBuffer->GetMemory() == nullptr) {
                MEDIA_LOG_W("dash PullDataWithoutCache, Read data from cache data success. update cache data fail.");
                return true;
            }
            MEDIA_LOG_I("dash PullDataWithoutCache merge before: cache offset: " PUBLIC_LOG_U64
                ", cache size: " PUBLIC_LOG_ZU, cacheDataMap_[streamID].GetOffset(),
                cacheDataMap_[streamID].GetData()->GetMemory()->GetSize());
            mergedBuffer->GetMemory()->Write(memory->GetReadOnlyData(), memory->GetSize(), 0);
            mergedBuffer->GetMemory()->Write(bufferPtr->GetMemory()->GetReadOnlyData(),
                bufferPtr->GetMemory()->GetSize(), memory->GetSize());
            cacheDataMap_[streamID].SetData(mergedBuffer);
            MEDIA_LOG_I("dash PullDataWithoutCache merge after: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
                ", cache size: " PUBLIC_LOG_ZU, offset, cacheDataMap_[streamID].GetOffset(),
                cacheDataMap_[streamID].GetData()->GetMemory()->GetSize());
        }
    } else {
        CacheData cacheTmp;
        cacheDataMap_[streamID] = cacheTmp;
    }
    if (cacheDataMap_[streamID].GetData() == nullptr || cacheDataMap_[streamID].GetData()->GetMemory() == nullptr) {
        MEDIA_LOG_I("PullDataWithoutCache, write cache data.");
        if (bufferPtr->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullDataWithoutCache, write cache data error. memory is nullptr!");
        } else {
            auto buffer = Buffer::CreateDefaultBuffer(bufferPtr->GetMemory()->GetSize());
            if (buffer != nullptr && buffer->GetMemory() != nullptr) {
                buffer->GetMemory()->Write(bufferPtr->GetMemory()->GetReadOnlyData(),
                    bufferPtr->GetMemory()->GetSize(), 0);
                cacheDataMap_[streamID].Init(buffer, offset);
                MEDIA_LOG_I("PullDataWithoutCache, write cache data success. offset=" PUBLIC_LOG_U64, offset);
            } else {
                MEDIA_LOG_W("PullDataWithoutCache, write cache data failed. memory is nullptr!");
            }
        }
    }
    return ret == Status::OK;
}

Status LiveHttpStreamDemuxer::ReadRetry(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Plugins::Buffer>& data)
{
    Status err = Status::OK;
    int32_t retryTimes = 0;
    while (true) {
        err = source_->Read(streamID, data, offset, size);
        if (err != Status::END_OF_STREAM && data->GetMemory()->GetSize() == 0) {
            OSAL::SleepFor(TRY_READ_SLEEP_TIME);
            retryTimes++;
            if (retryTimes > TRY_READ_TIMES) {
                break;
            }
            continue;
        }
        break;
    }
    return err;
}

Status LiveHttpStreamDemuxer::PullData(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Plugins::Buffer>& data)
{
    MEDIA_LOG_DD("IN, offset: " PUBLIC_LOG_U64 ", size: " PUBLIC_LOG_ZU, offset, size);
    if (!source_) {
        return Status::ERROR_INVALID_OPERATION;
    }
    
    auto readSize = size;
    Status err = ReadRetry(streamID, offset, readSize, data);
    FALSE_LOG_MSG(err == Status::OK, "hls, plugin read failed.");
    return err;
}

Status LiveHttpStreamDemuxer::ResetCache(int32_t streamID)
{
    if (cacheDataMap_.find(streamID) != cacheDataMap_.end()) {
        cacheDataMap_[streamID].Reset();
        cacheDataMap_.erase(streamID);
    }
    return Status::OK;
}

Status LiveHttpStreamDemuxer::ResetAllCache()
{
    for (auto& iter : cacheDataMap_) {
        iter.second.Reset();
    }
    cacheDataMap_.clear();
    return Status::OK;
}

Status LiveHttpStreamDemuxer::Start()
{
    return Status::OK;
}


Status LiveHttpStreamDemuxer::Stop()
{
    return Status::OK;
}


Status LiveHttpStreamDemuxer::Resume()
{
    return Status::OK;
}


Status LiveHttpStreamDemuxer::Pause()
{
    return Status::OK;
}

Status LiveHttpStreamDemuxer::Flush()
{
    return Status::OK;
}

Status LiveHttpStreamDemuxer::HandleReadHeader(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_HEADER, offset: " PUBLIC_LOG_D64
        ", expectedLen: " PUBLIC_LOG_ZU, offset, expectedLen);
    if (getRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer)) {
        if (IsDash()) {
            if (buffer != nullptr && buffer->streamID != streamID) {
                SetNewVideoStreamID(buffer->streamID);
                MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_HEADER, dash change, oldStreamID = " PUBLIC_LOG_D32
                    ", newStreamID = " PUBLIC_LOG_D32, streamID, buffer->streamID);
                return Status::END_OF_STREAM;
            }
        }
        DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
        return Status::OK;
    }
    if (expectedLen == 0) {
        return Status::END_OF_STREAM;
    }
    return Status::ERROR_NOT_ENOUGH_DATA;
}

Status LiveHttpStreamDemuxer::HandleReadPacket(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_FRAME");
    if (getRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer)) {
        if (IsDash()) {
            if (buffer != nullptr && buffer->streamID != streamID) {
                SetNewVideoStreamID(buffer->streamID);
                MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME, dash change, oldStreamID = " PUBLIC_LOG_D32
                    ", newStreamID = " PUBLIC_LOG_D32, streamID, buffer->streamID);
                return Status::END_OF_STREAM;
            }
        }
        DUMP_BUFFER2LOG("Demuxer GetRange", buffer, offset);
        DUMP_BUFFER2FILE(DEMUXER_INPUT_GET, buffer);
        if (buffer != nullptr && buffer->GetMemory() != nullptr &&
            buffer->GetMemory()->GetSize() == 0) {
            MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME in pausing(isIgnoreParse),"
                        " Read fail and try again");
            if (isIgnoreParse_.load()) {
                return Status::ERROR_WRONG_STATE;
            } else {
                return Status::ERROR_AGAIN;
            }
        }
        return Status::OK;
    }
    MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME, Status::END_OF_STREAM");
    return Status::END_OF_STREAM;
}

Status LiveHttpStreamDemuxer::CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    FALSE_RETURN_V(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE);
    switch (pluginStateMap_[streamID]) {
        case DemuxerState::DEMUXER_STATE_NULL:
            return Status::ERROR_WRONG_STATE;
        case DemuxerState::DEMUXER_STATE_PARSE_HEADER: {
            auto ret = HandleReadHeader(streamID, offset, buffer, expectedLen);
            if (ret != Status::OK) {
                return ret;
            }
            break;
        }
        case DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME:
        case DemuxerState::DEMUXER_STATE_PARSE_FRAME: {
            auto ret = HandleReadPacket(streamID, offset, buffer, expectedLen);
            if (ret != Status::OK) {
                return ret;
            }
            if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME) {
                SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
            }
            break;
        }
        default:
            break;
    }
    return Status::OK;
}

} // namespace Media
} // namespace OHOS
