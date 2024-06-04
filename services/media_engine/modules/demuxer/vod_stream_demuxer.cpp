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

#define HST_LOG_TAG "VodStreamDemuxer"

#include "vod_stream_demuxer.h"

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
#include "plugin/plugin_manager.h"
#include "plugin/plugin_time.h"
#include "source/source.h"

namespace OHOS {
namespace Media {
VodStreamDemuxer::VodStreamDemuxer()
    : cacheData_(),
    position_(0)
{
    MEDIA_LOG_I("VodStreamDemuxer called");
}

VodStreamDemuxer::~VodStreamDemuxer()
{
    MEDIA_LOG_I("~VodStreamDemuxer called");
    cacheData_.data = nullptr;
    cacheData_.offset = 0;
}

bool VodStreamDemuxer::GetPeekRangeSub(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    auto ret = PullData(offset, size, bufferPtr);
    if (ret == Status::ERROR_AGAIN) {
        isIgnoreRead_ = true;
        return true;
    } else {
        isIgnoreRead_ = false;
    }
    return Status::OK == ret;
}

bool VodStreamDemuxer::GetPeekRange(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    if (pluginState_.load() == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        if (bufferPtr) {
            return GetPeekRangeSub(offset, size, bufferPtr);
        }
    }
    MEDIA_LOG_D("PullMode, offset: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
        ", cache data: " PUBLIC_LOG_D32, offset, cacheData_.offset, (int32_t)(cacheData_.data != nullptr));
    if (cacheData_.data != nullptr && cacheData_.data->GetMemory() != nullptr &&
        offset >= cacheData_.offset && offset < (cacheData_.offset + cacheData_.data->GetMemory()->GetSize())) {
        auto memory = cacheData_.data->GetMemory();
        if (memory != nullptr && memory->GetSize() > 0) {
            MEDIA_LOG_D("PullMode, Read data from cache data.");
            return PullDataWithCache(offset, size, bufferPtr);
        }
    }
    return PullDataWithoutCache(offset, size, bufferPtr);
}

std::string VodStreamDemuxer::Init(std::string uri, uint64_t mediaDataSize)
{
    MediaAVCodec::AVCodecTrace trace("VodStreamDemuxer::Init");
    MEDIA_LOG_I("VodStreamDemuxer::Init called");
    InitTypeFinder();
    checkRange_ = [](uint64_t offset, uint32_t size) {
        return true;
    };
    peekRange_ = [this](uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr) -> bool {
        return GetPeekRange(offset, size, bufferPtr);
    };
    getRange_ = peekRange_;
    typeFinder_->Init(uri, mediaDataSize, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    MEDIA_LOG_I("PullMode FindMediaType result type: " PUBLIC_LOG_S ", uri: %{private}s, mediaDataSize: "
        PUBLIC_LOG_U64, type.c_str(), uri.c_str(), mediaDataSize);
    return type;
}

bool VodStreamDemuxer::PullDataWithCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    if (cacheData_.data == nullptr || cacheData_.data->GetMemory() == nullptr ||
        offset < cacheData_.offset || offset >= (cacheData_.offset + cacheData_.data->GetMemory()->GetSize())) {
        return false;
    }
    auto memory = cacheData_.data->GetMemory();
    if (memory == nullptr || memory->GetSize() <= 0) {
        return false;
    }

    MEDIA_LOG_D("PullMode, Read data from cache data.");
    uint64_t offsetInCache = offset - cacheData_.offset;
    if (size <= memory->GetSize() - offsetInCache) {
        bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, size, 0);
        return true;
    }
    bufferPtr->GetMemory()->Write(memory->GetReadOnlyData() + offsetInCache, memory->GetSize() - offsetInCache, 0);
    uint64_t remainOffset = cacheData_.offset + memory->GetSize();
    uint64_t remainSize = size - (memory->GetSize() - offsetInCache);
    std::shared_ptr<Buffer> tempBuffer = Buffer::CreateDefaultBuffer(remainSize);
    if (tempBuffer == nullptr || tempBuffer->GetMemory() == nullptr) {
        MEDIA_LOG_W("PullMode, Read data from cache data. only get partial data.");
        return true;
    }
    Status ret = PullData(remainOffset, remainSize, tempBuffer);
    if (ret == Status::OK) {
        bufferPtr->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize() - offsetInCache);
        std::shared_ptr<Buffer> mergedBuffer = Buffer::CreateDefaultBuffer(
            tempBuffer->GetMemory()->GetSize() + memory->GetSize());
        if (mergedBuffer == nullptr || mergedBuffer->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullMode, Read data from cache data success. update cache data fail.");
            return true;
        }
        mergedBuffer->GetMemory()->Write(memory->GetReadOnlyData(), memory->GetSize(), 0);
        mergedBuffer->GetMemory()->Write(tempBuffer->GetMemory()->GetReadOnlyData(),
            tempBuffer->GetMemory()->GetSize(), memory->GetSize());
        cacheData_.data = mergedBuffer;
        MEDIA_LOG_I("PullMode, offset: " PUBLIC_LOG_U64 ", cache offset: " PUBLIC_LOG_U64
            ", cache size: " PUBLIC_LOG_ZU, offset, cacheData_.offset,
            cacheData_.data->GetMemory()->GetSize());
    }
    return ret == Status::OK;
}

bool VodStreamDemuxer::PullDataWithoutCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr)
{
    Status ret = PullData(offset, size, bufferPtr);
    if ((cacheData_.data == nullptr || cacheData_.data->GetMemory() == nullptr) && ret == Status::OK) {
        MEDIA_LOG_I("PullMode, write cache data.");
        if (bufferPtr->GetMemory() == nullptr) {
            MEDIA_LOG_W("PullMode, write cache data error. memory is nullptr!");
        } else {
            auto buffer = Buffer::CreateDefaultBuffer(bufferPtr->GetMemory()->GetSize());
            if (buffer != nullptr && buffer->GetMemory() != nullptr) {
                buffer->GetMemory()->Write(bufferPtr->GetMemory()->GetReadOnlyData(),
                    bufferPtr->GetMemory()->GetSize(), 0);
                cacheData_.data = buffer;
                cacheData_.offset = offset;
                MEDIA_LOG_I("PullMode, write cache data success.");
            } else {
                MEDIA_LOG_W("PullMode, write cache data failed. memory is nullptr!");
            }
        }
    }
    return ret == Status::OK;
}

Status VodStreamDemuxer::PullData(uint64_t offset, size_t size, std::shared_ptr<Plugins::Buffer>& data)
{
    MEDIA_LOG_DD("IN, offset: " PUBLIC_LOG_U64 ", size: " PUBLIC_LOG_ZU
        ", position: " PUBLIC_LOG_U64, offset, size, position_);
    if (!source_) {
        return Status::ERROR_INVALID_OPERATION;
    }
    Status err;
    auto readSize = size;
    if (source_->IsSeekToTimeSupported()) {
        err = source_->ReadData(data, offset, readSize);
        FALSE_LOG_MSG(err == Status::OK, "hls, plugin read failed.");
        return err;
    }

    uint64_t totalSize = 0;
    if ((source_->GetSize(totalSize) == Status::OK) && (totalSize != 0)) {
        if (offset >= totalSize) {
            MEDIA_LOG_W("Offset: " PUBLIC_LOG_U64 " is larger than totalSize: " PUBLIC_LOG_U64, offset, totalSize);
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
    if (position_ != offset) {
        err = source_->SeekTo(offset);
        FALSE_RETURN_V_MSG_E(err == Status::OK, err, "Seek to " PUBLIC_LOG_U64 " fail", offset);
        position_ = offset;
    }

    err = source_->ReadData(data, offset, readSize);
    if (err == Status::OK) {
        position_ += data->GetMemory()->GetSize();
    }
    return err;
}

Status VodStreamDemuxer::Reset()
{
    cacheData_.data = nullptr;
    cacheData_.offset = 0;
    return Status::OK;
}

Status VodStreamDemuxer::Start()
{
    return Status::OK;
}


Status VodStreamDemuxer::Stop()
{
    return Status::OK;
}


Status VodStreamDemuxer::Resume()
{
    return Status::OK;
}


Status VodStreamDemuxer::Pause()
{
    return Status::OK;
}

Status VodStreamDemuxer::Flush()
{
    return Status::OK;
}

Status VodStreamDemuxer::CallbackReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen)
{
    switch (pluginState_.load()) {
        case DemuxerState::DEMUXER_STATE_NULL:
            return Status::ERROR_WRONG_STATE;
        case DemuxerState::DEMUXER_STATE_PARSE_HEADER: {
            MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_HEADER, offset: " PUBLIC_LOG_D64
                ", expectedLen: " PUBLIC_LOG_ZU, offset, expectedLen);
            if (getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
            } else if (0 == expectedLen) {
                return Status::END_OF_STREAM;
            } else {
                return Status::ERROR_NOT_ENOUGH_DATA;
            }
            break;
        }
        case DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME:
        case DemuxerState::DEMUXER_STATE_PARSE_FRAME: {
            MEDIA_LOG_D("Demuxer parse DEMUXER_STATE_PARSE_FRAME");
            if (getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2LOG("Demuxer GetRange", buffer, offset);
                DUMP_BUFFER2FILE(DEMUXER_INPUT_GET, buffer);
                if (isIgnoreParse_.load() && buffer->GetMemory()->GetSize() == 0) {
                    MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME in pausing(isIgnoreParse),"
                                " Read fail and try again");
                    return Status::ERROR_WRONG_STATE;
                } else if (buffer->GetMemory()->GetSize() == 0) {
                    return Status::ERROR_AGAIN;
                }
            } else {
                MEDIA_LOG_I("Demuxer parse DEMUXER_STATE_PARSE_FRAME, Status::END_OF_STREAM");
                return Status::END_OF_STREAM;
            }
            if (pluginState_.load() == DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME) {
                SetDemuxerState(DemuxerState::DEMUXER_STATE_PARSE_FRAME);
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
