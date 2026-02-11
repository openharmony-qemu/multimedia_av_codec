/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "StreamDemuxer"

#include "stream_demuxer.h"

#include <algorithm>
#include <map>
#include <memory>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "common/log.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "source/source.h"
#include "scoped_timer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "StreamDemuxer" };
}

namespace OHOS {
namespace Media {

const int32_t TRY_READ_SLEEP_TIME = 10;  //ms
const int32_t TRY_READ_TIMES = 10;
constexpr int64_t SOURCE_READ_WARNING_MS = 100;
StreamDemuxer::StreamDemuxer() : position_(0)
{
    MEDIA_LOG_I("VodStreamDemuxer called");
}

StreamDemuxer::~StreamDemuxer()
{
    MEDIA_LOG_I("~VodStreamDemuxer called");
    ResetAllCache();
}

Status StreamDemuxer::ReadFrameData(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    std::unique_lock<std::mutex> lock(cacheDataMutex_);
    if (IsDash() || GetIsDataSrcNoSeek()) {
        MEDIA_LOG_D("GetPeekRange read cache, offset: " PUBLIC_LOG_U64 " streamID: " PUBLIC_LOG_D32, offset, streamID);
        if (cacheDataMap_.find(streamID) != cacheDataMap_.end() && cacheDataMap_[streamID].CheckCacheExist(offset)) {
            MEDIA_LOG_D("GetPeekRange read cache, offset: " PUBLIC_LOG_U64, offset);
            auto memory = cacheDataMap_[streamID].GetData()->GetMemory();
            if (memory != nullptr && memory->GetSize() > 0) {
                MEDIA_LOG_D("GetPeekRange read cache, Read data from cache data. streamID: " PUBLIC_LOG_D32, streamID);
                return PullDataWithCache(streamID, offset, size, bufferPtr);
            }
        }
    }
    return PullData(streamID, offset, size, bufferPtr);
}

Status StreamDemuxer::ReadHeaderData(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    std::unique_lock<std::mutex> lock(cacheDataMutex_);
    if (cacheDataMap_.find(streamID) != cacheDataMap_.end() && cacheDataMap_[streamID].CheckCacheExist(offset)) {
        MEDIA_LOG_DD("GetPeekRange read cache, offset: " PUBLIC_LOG_U64, offset);
        auto memory = cacheDataMap_[streamID].GetData()->GetMemory();
        if (memory != nullptr && memory->GetSize() > 0) {
            MEDIA_LOG_D("GetPeekRange read cache, Read data from cache data.");
            return PullDataWithCache(streamID, offset, size, bufferPtr);
        }
    }
    return PullDataWithoutCache(streamID, offset, size, bufferPtr);
}

Status StreamDemuxer::GetPeekRange(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE,
        "GetPeekRange interrupt " PUBLIC_LOG_D32 " " PUBLIC_LOG_U64 " " PUBLIC_LOG_ZU, streamID, offset, size);
    FALSE_RETURN_V_MSG_E(streamID >= 0, Status::ERROR_INVALID_PARAMETER,
        "Invalid streamId, id = " PUBLIC_LOG_D32, streamID);
    if (bufferPtr == nullptr) {
        MEDIA_LOG_E("GetPeekRange bufferPtr invalid.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    bufferPtr->streamID = streamID;
    Status ret = Status::OK;
    if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        ret = ReadFrameData(streamID, offset, size, bufferPtr);
    } else {
        ret = ReadHeaderData(streamID, offset, size, bufferPtr);
    }
    if (ret != Status::OK) {
        return ret;
    }
    return CheckChangeStreamID(streamID, bufferPtr);
}

Status StreamDemuxer::Init(const std::string& uri)
{
    MediaAVCodec::AVCodecTrace trace("StreamDemuxer::Init");
    MEDIA_LOG_I("StreamDemuxer::Init called");
    checkRange_ = [](int32_t streamID, uint64_t offset, uint32_t size) {
        return Status::OK;
    };
    peekRange_ = [this](int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> Status {
        return GetPeekRange(streamID, offset, size, bufferPtr);
    };
    getRange_ = peekRange_;
    uri_ = uri;
    return Status::OK;
}

Status StreamDemuxer::PullDataWithCache(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    FALSE_RETURN_V_MSG_E(bufferPtr->GetMemory() != nullptr, Status::ERROR_UNKNOWN, "bufferPtr invalid");
    auto memory = cacheDataMap_[streamID].GetData()->GetMemory();
    FALSE_RETURN_V_MSG_E(memory != nullptr, Status::ERROR_UNKNOWN, "memory invalid");
    MEDIA_LOG_D("PullDataWithCache, Read data from cache data. streamID: " PUBLIC_LOG_D32, streamID);
    uint64_t offsetInCache = offset - cacheDataMap_[streamID].GetOffset();
    if (size <= memory->GetSize() - offsetInCache) {
        MEDIA_LOG_D("Readfromcache. streamID: " PUBLIC_LOG_D32, streamID);
        bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, size, 0);
        return Status::OK;
    }
    bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, memory->GetSize() - offsetInCache, 0);
    uint64_t remainOffset = cacheDataMap_[streamID].GetOffset() + memory->GetSize();
    uint64_t remainSize = size - (memory->GetSize() - offsetInCache);
    std::shared_ptr<Buffer> tempBuffer = Buffer::CreateDefaultBuffer(remainSize);
    if (tempBuffer == nullptr || tempBuffer->GetMemory() == nullptr) {
        MEDIA_LOG_W("PullDataWithCache, Read data from cache data. only get partial data.");
        return Status::ERROR_UNKNOWN;
    }
    Status ret = PullData(streamID, remainOffset, remainSize, tempBuffer);
    if (ret == Status::OK) {
        FALSE_RETURN_V_MSG_E(tempBuffer->GetMemory() != nullptr, Status::ERROR_UNKNOWN, "tempBuffer invalid");
        bufferPtr->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize() - offsetInCache);
        if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
            MEDIA_LOG_W("PullDataWithCache, not cache begin.");
            return ret;
        }
        std::shared_ptr<Buffer> mergedBuffer = Buffer::CreateDefaultBuffer(
            tempBuffer->GetMemory()->GetSize() + memory->GetSize());
        FALSE_RETURN_V_MSG_E(mergedBuffer != nullptr, Status::ERROR_UNKNOWN, "mergedBuffer invalid");
        FALSE_RETURN_V_MSG_E(mergedBuffer->GetMemory() != nullptr, Status::ERROR_UNKNOWN,
            "mergedBuffer->GetMemory invalid");
        mergedBuffer->GetMemory()->Write(memory->GetReadOnlyData(), memory->GetSize(), 0);
        mergedBuffer->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize());
        cacheDataMap_[streamID].SetData(mergedBuffer);
        memory = cacheDataMap_[streamID].GetData()->GetMemory();
        FALSE_RETURN_V_MSG_E(memory != nullptr, Status::ERROR_UNKNOWN, "memory invalid");
        MEDIA_LOG_I("PullDataWithCache, offset: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
            ", cache size: " PUBLIC_LOG_ZU, offset, cacheDataMap_[streamID].GetOffset(), memory->GetSize());
    }
    FALSE_RETURN_V_MSG_E(ret != Status::END_OF_STREAM, Status::OK, "eos&data return data");
    return ret;
}

Status StreamDemuxer::ProcInnerDash(int32_t streamID,  uint64_t offset, std::shared_ptr<Buffer>& bufferPtr)
{
    FALSE_RETURN_V_MSG_E(bufferPtr != nullptr, Status::ERROR_UNKNOWN, "bufferPtr invalid");
    if (IsDash()) {
        MEDIA_LOG_D("dash PullDataWithoutCache, cacheDataMap_ exist streamID , merge it.");
        FALSE_RETURN_V_MSG_E(cacheDataMap_[streamID].GetData() != nullptr, Status::ERROR_UNKNOWN, "getdata invalid");
        auto cacheMemory = cacheDataMap_[streamID].GetData()->GetMemory();
        auto bufferMemory = bufferPtr->GetMemory();
        FALSE_RETURN_V_MSG_E(bufferMemory != nullptr, Status::ERROR_UNKNOWN, "bufferPtr invalid");
        FALSE_RETURN_V_MSG_E(cacheMemory != nullptr, Status::ERROR_UNKNOWN, "cacheMemory invalid");
        std::shared_ptr<Buffer> mergedBuffer = Buffer::CreateDefaultBuffer(
            bufferMemory->GetSize() + cacheMemory->GetSize());
        FALSE_RETURN_V_MSG_E(mergedBuffer != nullptr, Status::ERROR_UNKNOWN, "mergedBuffer invalid");
        auto mergeMemory = mergedBuffer->GetMemory();
        FALSE_RETURN_V_MSG_E(mergeMemory != nullptr, Status::ERROR_UNKNOWN, "mergeMemory invalid");
        MEDIA_LOG_D("dash PullDataWithoutCache merge before: cache offset: " PUBLIC_LOG_U64
            ", cache size: " PUBLIC_LOG_ZU, cacheDataMap_[streamID].GetOffset(), cacheMemory->GetSize());
        mergeMemory->Write(cacheMemory->GetReadOnlyData(), cacheMemory->GetSize(), 0);
        mergeMemory->Write(bufferMemory->GetReadOnlyData(), bufferMemory->GetSize(), cacheMemory->GetSize());
        cacheDataMap_[streamID].SetData(mergedBuffer);
        MEDIA_LOG_D("dash PullDataWithoutCache merge after: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64,
            offset, cacheDataMap_[streamID].GetOffset());
    }
    return Status::OK;
}

Status StreamDemuxer::PullDataWithoutCache(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& bufferPtr)
{
    Status ret = PullData(streamID, offset, size, bufferPtr);
    if (ret != Status::OK) {
        MEDIA_LOG_E("PullDataWithoutCache, PullData error " PUBLIC_LOG_D32, static_cast<int32_t>(ret));
        return ret;
    }
    if (cacheDataMap_.find(streamID) != cacheDataMap_.end()) {
        MEDIA_LOG_D("PullDataWithoutCache, cacheDataMap_ exist streamID , do nothing.");
        ret = ProcInnerDash(streamID, offset, bufferPtr);
        if (ret != Status::OK) {
            MEDIA_LOG_E("ProcInnerDash error " PUBLIC_LOG_D32, static_cast<int32_t>(ret));
            return ret;
        }
    } else {
        CacheData cacheTmp;
        cacheDataMap_[streamID] = cacheTmp;
    }
    if (cacheDataMap_[streamID].GetData() == nullptr || cacheDataMap_[streamID].GetData()->GetMemory() == nullptr) {
        MEDIA_LOG_D("PullDataWithoutCache, write cache data.");
        if (bufferPtr->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullDataWithoutCache, write cache data error. memory is nullptr!");
        } else {
            auto buffer = Buffer::CreateDefaultBuffer(bufferPtr->GetMemory()->GetSize());
            if (buffer != nullptr && buffer->GetMemory() != nullptr) {
                buffer->GetMemory()->Write(bufferPtr->GetMemory()->GetReadOnlyData(),
                    bufferPtr->GetMemory()->GetSize(), 0);
                cacheDataMap_[streamID].Init(buffer, offset);
                MEDIA_LOG_D("PullDataWithoutCache, write cache data success. offset=" PUBLIC_LOG_U64, offset);
            } else {
                MEDIA_LOG_W("PullDataWithoutCache, write cache data failed. memory is nullptr!");
            }
        }
    }
    return ret;
}

Status StreamDemuxer::ReadRetry(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Plugins::Buffer>& data)
{
    FALSE_RETURN_V_MSG_E(data->GetMemory() != nullptr, Status::ERROR_UNKNOWN, "getmemory invalid");
    Status err = Status::OK;
    int32_t retryTimes = 0;
    while (true && !isInterruptNeeded_.load()) {
        {
            ScopedTimer timer("Source Read", SOURCE_READ_WARNING_MS);
            err = source_->Read(streamID, data, offset, size);
        }
        if (IsDash() && streamID != data->streamID) {
            break;
        }
        FALSE_RETURN_V_MSG_E(err != Status::ERROR_UNKNOWN, Status::ERROR_UNKNOWN, "error unknown");
        if (err != Status::END_OF_STREAM && data->GetMemory()->GetSize() == 0) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                readCond_.wait_for(lock, std::chrono::milliseconds(TRY_READ_SLEEP_TIME),
                                   [&] { return isInterruptNeeded_.load(); });
            }
            retryTimes++;
            if (retryTimes > TRY_READ_TIMES || isInterruptNeeded_.load()) {
                break;
            }
            continue;
        }
        break;
    }
    FALSE_LOG_MSG(!isInterruptNeeded_.load(), "ReadRetry interrupted");
    return err;
}

Status StreamDemuxer::PullData(int32_t streamID, uint64_t offset, size_t size,
    std::shared_ptr<Plugins::Buffer>& data)
{
    MEDIA_LOG_DD("IN, offset: " PUBLIC_LOG_U64 ", size: " PUBLIC_LOG_ZU
        ", position: " PUBLIC_LOG_U64, offset, size, position_);
    if (!source_) {
        return Status::ERROR_INVALID_OPERATION;
    }
    Status err;
    auto readSize = size;
    if (source_->IsSeekToTimeSupported() || source_->GetSeekable() == Plugins::Seekable::UNSEEKABLE) {
        err = ReadRetry(streamID, offset, readSize, data);
        FALSE_LOG_MSG(err == Status::OK, "hls, plugin read failed.");
        return err;
    }

    uint64_t totalSize = 0;
    if ((source_->GetSize(totalSize) == Status::OK) && (totalSize != 0)) {
        if (offset >= totalSize) {
            MEDIA_LOG_D("Offset: " PUBLIC_LOG_U64 " is larger than totalSize: " PUBLIC_LOG_U64, offset, totalSize);
            return Status::END_OF_STREAM;
        }
        if ((offset + readSize) > totalSize) {
            readSize = totalSize - offset;
        }
        if (data->GetMemory() != nullptr) {
            auto realSize = data->GetMemory()->GetCapacity();
            readSize = (readSize > realSize) ? realSize : readSize;
        }
        MEDIA_LOG_DD("TotalSize_: " PUBLIC_LOG_U64, totalSize);
    }

    if (position_ != offset || GetIsDataSrc()) {
        err = source_->SeekTo(offset);
        FALSE_RETURN_V_MSG_E(err == Status::OK, err, "Seek to " PUBLIC_LOG_U64 " fail", offset);
        position_ = offset;
    }

    err = ReadRetry(streamID, offset, readSize, data);
    if (err == Status::OK) {
        FALSE_RETURN_V_MSG_E(data->GetMemory() != nullptr, Status::ERROR_UNKNOWN, "data->GetMemory invalid");
        position_ += data->GetMemory()->GetSize();
    }
    return err;
}

Status StreamDemuxer::ResetCache(int32_t streamID)
{
    std::unique_lock<std::mutex> lock(cacheDataMutex_);
    if (cacheDataMap_.find(streamID) != cacheDataMap_.end()) {
        cacheDataMap_[streamID].Reset();
        cacheDataMap_.erase(streamID);
    }
    return Status::OK;
}

Status StreamDemuxer::ResetAllCache()
{
    std::unique_lock<std::mutex> lock(cacheDataMutex_);
    for (auto& iter : cacheDataMap_) {
        iter.second.Reset();
    }
    cacheDataMap_.clear();
    return Status::OK;
}

Status StreamDemuxer::Start()
{
    return Status::OK;
}


Status StreamDemuxer::Stop()
{
    return Status::OK;
}


Status StreamDemuxer::Resume()
{
    return Status::OK;
}


Status StreamDemuxer::Pause()
{
    return Status::OK;
}

Status StreamDemuxer::Flush()
{
    return Status::OK;
}

Status StreamDemuxer::HandleReadHeader(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_HEADER, offset: " PUBLIC_LOG_D64
        ", expectedLen: " PUBLIC_LOG_ZU, offset, expectedLen);
    if (expectedLen == 0) {
        return Status::END_OF_STREAM;
    }
    Status ret = getRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer);
    if (ret == Status::OK) {
        DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
        return ret;
    }
    // Under the current specifications, change buffer->streamID only in the scenario of switching tracks.
    FALSE_RETURN_V_NOLOG(!IsDash() || buffer == nullptr || buffer->streamID == streamID, Status::END_OF_STREAM);

    MEDIA_LOG_W("Demuxer parse DEMUXER_STATE_PARSE_HEADER, getRange_ failed, ret = " PUBLIC_LOG_D32, ret);
    return ret;
}

Status StreamDemuxer::CheckChangeStreamID(int32_t streamID, std::shared_ptr<Buffer>& buffer)
{
    if (IsDash()) {
        if (buffer != nullptr && buffer->streamID != streamID) {
            if (GetNewVideoStreamID() == streamID) {
                SetNewVideoStreamID(buffer->streamID);
            } else if (GetNewAudioStreamID() == streamID) {
                SetNewAudioStreamID(buffer->streamID);
            } else if (GetNewSubtitleStreamID() == streamID) {
                SetNewSubtitleStreamID(buffer->streamID);
            } else {}
            MEDIA_LOG_I("Demuxer parse dash change, oldStreamID = " PUBLIC_LOG_D32
                ", newStreamID = " PUBLIC_LOG_D32, streamID, buffer->streamID);
            return Status::END_OF_STREAM;
        }
    }
    return Status::OK;
}

Status StreamDemuxer::HandleReadPacket(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
    size_t expectedLen)
{
    MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_FRAME");
    Status ret = getRange_(streamID, static_cast<uint64_t>(offset), expectedLen, buffer);
    if (ret == Status::OK) {
        DUMP_BUFFER2LOG("Demuxer GetRange", buffer, offset);
        DUMP_BUFFER2FILE(DEMUXER_INPUT_GET, buffer);
        if (buffer != nullptr && buffer->GetMemory() != nullptr &&
            buffer->GetMemory()->GetSize() == 0) {
            MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME in pausing(isIgnoreParse),"
                        " Read fail and try again");
            return Status::ERROR_AGAIN;
        }
        return ret;
    }
    MEDIA_LOG_W("Demuxer parse DEMUXER_STATE_PARSE_FRAME, getRange_ failed, ret = " PUBLIC_LOG_D32, ret);
    return ret;
}

Status StreamDemuxer::CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
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
            if (ret == Status::END_OF_STREAM &&
                pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME) {
                SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
                return ret;
            }
            if (ret != Status::OK) {
                return ret;
            }
            if (pluginStateMap_[streamID] == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME) {
                SetDemuxerState(streamID, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
                if (firstFrameDecapsulationTime_ == 0) {
                    auto duration = std::chrono::steady_clock::now().time_since_epoch();
                    firstFrameDecapsulationTime_ =
                        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                }
            }
            break;
        }
        default:
            break;
    }
    return Status::OK;
}

void StreamDemuxer::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("StreamDemuxer OnInterrupted %{public}d", isInterruptNeeded);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isInterruptNeeded_ = isInterruptNeeded;
        readCond_.notify_all();
    }
    TypeFinderInterrupt(isInterruptNeeded);
}

int64_t StreamDemuxer::GetFirstFrameDecapsulationTime()
{
    return firstFrameDecapsulationTime_;
}
} // namespace Media
} // namespace OHOS
