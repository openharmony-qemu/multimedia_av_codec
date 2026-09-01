/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#define HST_LOG_TAG "DemuxerDataReader"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "demuxer_data_reader.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "DemuxerDataReader" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr uint8_t BYTE_LENGTH = 8;
constexpr uint8_t READ_RETRY_TIMES = 10;
constexpr uint32_t READ_RETRY_SLEEP_TIME_US = 5000;
constexpr size_t READ_CACHE_SIZE = 4096;
const uint64_t INIT_DOWNLOADS_DATA_SIZE_THRESHOLD = 2 * 1024 * 1024;
constexpr int64_t LOG_INTERVAL_MS = 2000; // 2s
constexpr uint32_t LOG_MAX_COUNT = 10; // 10 times

Status DemuxerDataReader::SetDataReader(const std::shared_ptr<DataSource>& source)
{
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "Source is nullptr");

    dataSource_ = source;
    readCache_.clear();
    readCacheOffset_ = -1;
    readCacheSize_ = 0;
    MEDIA_LOG_D("SetDataReader Finish");
    return Status::OK;
}

Status DemuxerDataReader::SetReadTimeoutForInitSeek(int64_t timeoutMs)
{
    FALSE_RETURN_V_MSG_E(timeoutMs >= 0, Status::ERROR_INVALID_PARAMETER,
        "Read timeout must be non-negative");
    readTimeoutConfigured_ = true;
    readTimeoutMs_ = timeoutMs;
    readStartTimeMs_ = 0;
    return Status::OK;
}

bool DemuxerDataReader::IsReadTimeoutExpired()
{
    if (!readTimeoutConfigured_ || readTimeoutMs_ <= 0) {
        return false;
    }
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (readStartTimeMs_ <= 0) {
        readStartTimeMs_ = now;
        return false;
    }
    if (now - readStartTimeMs_ > readTimeoutMs_) {
        MEDIA_LOG_E("Read timeout, elapsed: " PUBLIC_LOG_D64 "ms, limit: " PUBLIC_LOG_D64 "ms",
            now - readStartTimeMs_, readTimeoutMs_);
        return true;
    }
    return false;
}

Status DemuxerDataReader::ReadUintData(int64_t offset, uint8_t* buffer, size_t size)
{
    FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_INVALID_PARAMETER, "Buffer is nullptr");
    FALSE_RETURN_V_MSG_E(size > 0, Status::ERROR_INVALID_PARAMETER, "Size must be greater than 0");
    FALSE_RETURN_V_MSG_E(offset >= 0, Status::ERROR_INVALID_PARAMETER, "Offset is invalid");

    if (size <= READ_CACHE_SIZE) {
        return ReadFromCache(offset, buffer, size);
    }

    return ReadFully(offset, buffer, size);
}

Status DemuxerDataReader::ReadFromCache(int64_t offset, uint8_t* buffer, size_t size)
{
    size_t copied = 0;
    while (copied < size) {
        int64_t currentOffset = offset + static_cast<int64_t>(copied);
        bool cacheHit = readCacheOffset_ >= 0 && currentOffset >= readCacheOffset_ &&
            currentOffset < readCacheOffset_ + static_cast<int64_t>(readCacheSize_);
        if (!cacheHit) {
            Status ret = FillReadCache(currentOffset);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Fill read cache failed");
            continue;
        }

        size_t cacheIndex = static_cast<size_t>(currentOffset - readCacheOffset_);
        size_t available = readCacheSize_ - cacheIndex;
        size_t copySize = std::min(size - copied, available);
        auto copyRet = memcpy_s(buffer + copied, copySize, readCache_.data() + cacheIndex, copySize);
        FALSE_RETURN_V_MSG_E(copyRet == EOK, Status::ERROR_UNKNOWN, "memcpy_s failed");
        copied += copySize;
    }

    return Status::OK;
}

Status DemuxerDataReader::FillReadCache(int64_t offset)
{
    if (readCache_.size() != READ_CACHE_SIZE) {
        readCache_.resize(READ_CACHE_SIZE);
    }

    auto bufferInfo = std::make_shared<Buffer>();
    FALSE_RETURN_V_MSG_E(bufferInfo != nullptr, Status::ERROR_NO_MEMORY, "bufferInfo is nullptr");
    auto bufferMemory = bufferInfo->WrapMemory(readCache_.data(), readCache_.size(), 0);
    FALSE_RETURN_V_MSG_E(bufferMemory != nullptr, Status::ERROR_NO_MEMORY, "Alloc memory failed");
    
    Status ret = Status::ERROR_AGAIN;
    uint32_t count = 0;
    bool hasProgress = false;
    MediaAVCodec::AVCodecTrace trace("AVReadPacket_ReadAt");
    while (ret == Status::ERROR_AGAIN &&
        (readTimeoutConfigured_ || count < READ_RETRY_TIMES)) {
        ret = dataSource_->ReadAt(offset, bufferInfo, readCache_.size());
        size_t readSize = bufferMemory->GetSize();
        UpdateInitDownloadData(readSize);
        if (readSize > 0) {
            readStartTimeMs_ = 0;
            hasProgress = true;
            break;
        }
        FALSE_BREAK_NOLOG(ret != Status::OK);
        FALSE_RETURN_V_NOLOG(ret != Status::ERROR_AGAIN || !IsReadTimeoutExpired(), Status::ERROR_UNKNOWN);
        ++count;
        std::this_thread::sleep_for(std::chrono::microseconds(READ_RETRY_SLEEP_TIME_US));
    }
    ret = hasProgress ? Status::OK : ret;
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");
    readCacheSize_ = bufferMemory->GetSize();
    FALSE_RETURN_V_MSG_E(readCacheSize_ > 0, Status::ERROR_AGAIN, "No enough data");
    readCacheSize_ = readCacheSize_ > readCache_.size() ? readCache_.size() : readCacheSize_;
    readCacheOffset_ = offset;

    return Status::OK;
}

Status DemuxerDataReader::ReadFully(int64_t offset, uint8_t* buffer, size_t size)
{
    FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_INVALID_PARAMETER, "Buffer is nullptr");
    FALSE_RETURN_V_MSG_E(size > 0, Status::ERROR_INVALID_PARAMETER, "Size must be greater than 0");
    FALSE_RETURN_V_MSG_E(offset >= 0, Status::ERROR_INVALID_PARAMETER, "Offset is invalid");

    size_t totalRead = 0;
    MediaAVCodec::AVCodecTrace trace("AVReadPacket_ReadFully");
    while (totalRead < size) {
        size_t remaining = size - totalRead;
        auto bufferInfo = std::make_shared<Buffer>();
        FALSE_RETURN_V_MSG_E(bufferInfo != nullptr, Status::ERROR_NO_MEMORY, "bufferInfo is nullptr");
        auto bufferMemory = bufferInfo->WrapMemory(buffer + totalRead, remaining, 0);
        FALSE_RETURN_V_MSG_E(bufferMemory != nullptr, Status::ERROR_NO_MEMORY, "Alloc memory failed");

        Status ret = Status::ERROR_AGAIN;
        uint32_t count = 0;
        bool hasProgress = false;
        while (ret == Status::ERROR_AGAIN && (readTimeoutConfigured_ || count < READ_RETRY_TIMES)) {
            ret = dataSource_->ReadAt(offset + static_cast<int64_t>(totalRead), bufferInfo, remaining);
            size_t readSize = bufferMemory->GetSize();
            UpdateInitDownloadData(readSize);
            if (readSize > 0) {
                readSize = readSize > remaining ? remaining : readSize;
                totalRead += readSize;
                readStartTimeMs_ = 0;
                hasProgress = true;
                break;
            }
            FALSE_BREAK_NOLOG(ret != Status::OK);
            FALSE_RETURN_V_NOLOG(ret != Status::ERROR_AGAIN || !IsReadTimeoutExpired(), Status::ERROR_UNKNOWN);
            ++count;
            std::this_thread::sleep_for(std::chrono::microseconds(READ_RETRY_SLEEP_TIME_US));
        }

        if (hasProgress) {
            continue;
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read data failed");
        FALSE_RETURN_V_MSG_E(bufferMemory->GetSize() > 0, Status::ERROR_AGAIN, "No enough data");
    }

    return Status::OK;
}

void DemuxerDataReader::UpdateInitDownloadData(size_t dataSize)
{
    if (!initCompleted_) {
        if (initDownloadDataSize_ <= UINT64_MAX - static_cast<uint64_t>(dataSize)) {
            initDownloadDataSize_ += static_cast<uint64_t>(dataSize);
        } else {
            AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGW, LOG_INTERVAL_MS, LOG_MAX_COUNT,
                "DataSize " PUBLIC_LOG_U64 " is invalid", static_cast<uint64_t>(dataSize));
        }
    }
}

void DemuxerDataReader::NotifyInitializationCompleted()
{
    initCompleted_ = true;
    if (initDownloadDataSize_ >= INIT_DOWNLOADS_DATA_SIZE_THRESHOLD) {
        MEDIA_LOG_I("Large init size " PUBLIC_LOG_U64, initDownloadDataSize_);
    }
}

bool DemuxerBitReader::HasBits(size_t numBits) const
{
    size_t totalBitsRead = 0;
    size_t totalBitsAvailable = 0;
    if (__builtin_mul_overflow(byteOffset_, BYTE_LENGTH, &totalBitsRead) ||
        __builtin_mul_overflow(totalBytes_, BYTE_LENGTH, &totalBitsAvailable)) {
        return false;
    }
    if (__builtin_add_overflow(totalBitsRead, bitOffset_, &totalBitsRead)) {
        return false;
    }

    return (totalBitsAvailable - totalBitsRead) >= numBits;
}

uint16_t DemuxerBitReader::ShowBits(uint8_t numBits)
{
    if (numBits < 0 || numBits > BYTE_LENGTH || !HasBits(numBits)) {
        return 0;
    }
    uint16_t value = 0;
    uint8_t bitOffset = bitOffset_;
    uint32_t byteOffset = byteOffset_;
    while (numBits > 0) {
        uint8_t bitsInCurrentByte = BYTE_LENGTH - bitOffset;
        uint8_t bitsToRead = std::min(numBits, bitsInCurrentByte);

        if (byteOffset >= totalBytes_) {
            return 0;
        }
        uint8_t currentByte = data_[byteOffset];
        uint8_t maskedBits = (currentByte >> (BYTE_LENGTH - bitOffset - bitsToRead)) & ((1 << bitsToRead) - 1);
        value |= (maskedBits << (numBits - bitsToRead));
        bitOffset += bitsToRead;
        if (bitOffset == BYTE_LENGTH) {
            bitOffset = 0;
            byteOffset++;
        }
        numBits -= bitsToRead;
    }

    return value;
}

uint8_t DemuxerBitReader::ReadBits(uint8_t numBits)
{
    FALSE_RETURN_V_NOLOG(numBits > 0, 0);
    FALSE_RETURN_V(numBits <= BYTE_LENGTH, 0);
    FALSE_RETURN_V(HasBits(numBits), 0);
    uint8_t value = 0;
    while (numBits > 0) {
        uint8_t bitsInCurrentByte = BYTE_LENGTH - bitOffset_;
        uint8_t bitsToRead = std::min(numBits, bitsInCurrentByte);

        if (byteOffset_ >= totalBytes_) {
            return 0;
        }
        uint8_t currentByte = data_[byteOffset_];
        uint8_t maskedBits = (currentByte >> (BYTE_LENGTH - bitOffset_ - bitsToRead)) & ((1 << bitsToRead) - 1);
        value |= (maskedBits << (numBits - bitsToRead));
        bitOffset_ += bitsToRead;
        if (bitOffset_ == BYTE_LENGTH) {
            bitOffset_ = 0;
            byteOffset_++;
        }
        numBits -= bitsToRead;
    }

    return value;
}

bool DemuxerBitReader::SkipBits(uint8_t numBits)
{
    if (numBits == 0) {
        return true;
    }
    if (!HasBits(numBits)) {
        return false;
    }
    while (numBits > 0 && HasBits(numBits)) {
        uint8_t bitsInCurrentByte = BYTE_LENGTH - bitOffset_;
        uint8_t bitsToSkip = std::min(numBits, bitsInCurrentByte);
        bitOffset_ += bitsToSkip;
        if (bitOffset_ == BYTE_LENGTH) {
            bitOffset_ = 0;
            byteOffset_++;
        }
        numBits -= bitsToSkip;
    }
    if (numBits > 0) {
        return false;
    }
    return true;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
