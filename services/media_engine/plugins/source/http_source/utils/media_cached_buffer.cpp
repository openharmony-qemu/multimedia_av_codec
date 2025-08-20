/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <mutex>
#include <list>
#include <algorithm>
#include <cassert>
#include <limits>
#include <securec.h>
#include "media_cached_buffer.h"
#include "common/log.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace OHOS {
namespace Media {
constexpr size_t CACHE_FRAGMENT_MAX_NUM_DEFAULT = 300; // Maximum number of fragment nodes
constexpr size_t CACHE_FRAGMENT_MAX_NUM_LARGE = 10; // Maximum number of fragment nodes
constexpr size_t CACHE_FRAGMENT_MIN_NUM_DEFAULT = 3; // Minimum number of fragment nodes
constexpr double NEW_FRAGMENT_INIT_CHUNK_NUM = 128.0; // Restricting the cache size of seek operation, 128 = 2MB
constexpr double NEW_FRAGMENT_NIT_DEFAULT_DENOMINATOR = 0.25;
constexpr double CACHE_RELEASE_FACTOR_DEFAULT = 10;
constexpr double TO_PERCENT = 100;
constexpr int64_t MAX_TOTAL_READ_SIZE = 2000000;
constexpr int64_t UP_LIMIT_MAX_TOTAL_READ_SIZE = 3000000;
constexpr int64_t ACCESS_OFFSET_MAX_LENGTH = 2 * 1024;

inline constexpr bool BoundedIntervalComp(int64_t mid, uint64_t start, int64_t end)
{
    return (static_cast<int64_t>(start) <= mid && mid <= end);
}

inline constexpr bool LeftBoundedRightOpenComp(int64_t mid, uint64_t start, int64_t end)
{
    return (static_cast<int64_t>(start) <= mid && mid < end);
}

inline void IncreaseStep(uint8_t*& src, uint64_t& offset, size_t& writeSize, size_t step)
{
    src += step;
    offset += static_cast<uint64_t>(step);
    writeSize += step;
}

inline void InitChunkInfo(CacheChunk& chunkInfo, uint64_t offset)
{
    chunkInfo.offset = offset;
    chunkInfo.dataLength = 0;
}

CacheMediaChunkBufferImpl::CacheMediaChunkBufferImpl()
    : totalBuffSize_(0), totalReadSize_(0), chunkMaxNum_(0), chunkSize_(0), bufferAddr_(nullptr),
      fragmentMaxNum_(CACHE_FRAGMENT_MAX_NUM_DEFAULT),
      lruCache_(CACHE_FRAGMENT_MAX_NUM_DEFAULT) {}

CacheMediaChunkBufferImpl::~CacheMediaChunkBufferImpl()
{
    std::lock_guard lock(mutex_);
    freeChunks_.clear();
    fragmentCacheBuffer_.clear();
    readPos_ = fragmentCacheBuffer_.end();
    writePos_ = fragmentCacheBuffer_.end();
    chunkMaxNum_ = 0;
    totalReadSize_ = 0;
    if (bufferAddr_ != nullptr) {
        free(bufferAddr_);
        bufferAddr_ = nullptr;
    }
}

bool CacheMediaChunkBufferImpl::Init(uint64_t totalBuffSize, uint32_t chunkSize)
{
    if (isLargeOffsetSpan_) {
        lruCache_.ReCacheSize(CACHE_FRAGMENT_MAX_NUM_LARGE);
    } else {
        lruCache_.ReCacheSize(CACHE_FRAGMENT_MAX_NUM_DEFAULT);
    }
    if (totalBuffSize == 0 || chunkSize == 0 || totalBuffSize < chunkSize) {
        return false;
    }
    double newFragmentInitChunkNum  = NEW_FRAGMENT_INIT_CHUNK_NUM;
    uint64_t diff = (totalBuffSize + chunkSize) > 1 ? (totalBuffSize + chunkSize) - 1 : 0;
    int64_t chunkNum = static_cast<int64_t>(diff / chunkSize) + 1;
    if ((chunkNum - static_cast<int64_t>(newFragmentInitChunkNum)) < 0 ||
        chunkNum > MAX_CACHE_BUFFER_SIZE) {
        return false;
    }
    if (newFragmentInitChunkNum > static_cast<double>(chunkNum) * NEW_FRAGMENT_NIT_DEFAULT_DENOMINATOR) {
        newFragmentInitChunkNum = std::max(1.0, static_cast<double>(chunkNum) * NEW_FRAGMENT_NIT_DEFAULT_DENOMINATOR);
    }
    std::lock_guard lock(mutex_);
    if (bufferAddr_ != nullptr) {
        return false;
    }
    readPos_ = fragmentCacheBuffer_.end();
    writePos_ = fragmentCacheBuffer_.end();
    size_t sizePerChunk = sizeof(CacheChunk) + chunkSize;
    int64_t totalSize = static_cast<int64_t>(sizePerChunk) * chunkNum;
    if (totalSize <= 0 || totalSize > MAX_CACHE_BUFFER_SIZE * CHUNK_SIZE) {
        return false;
    }
    bufferAddr_ = static_cast<uint8_t*>(malloc(totalSize));
    if (bufferAddr_ == nullptr) {
        return false;
    }
    uint8_t* temp = bufferAddr_;
    for (auto i = 0; i < chunkNum; ++i) {
        auto chunkInfo = reinterpret_cast<CacheChunk*>(temp);
        chunkInfo->offset = 0;
        chunkInfo->dataLength = 0;
        chunkInfo->chunkSize = static_cast<uint32_t>(chunkSize);
        freeChunks_.push_back(chunkInfo);
        temp += sizePerChunk;
    }
    chunkMaxNum_ = chunkNum >= 1 ? static_cast<uint32_t>(chunkNum) - 1 : 0; // -1
    totalBuffSize_ = totalBuffSize;
    chunkSize_ = chunkSize;
    initReadSizeFactor_ = newFragmentInitChunkNum / (chunkMaxNum_ - newFragmentInitChunkNum);
    loopInterruptClock_.Reset();
    return true;
}

// Upadate the chunk read from the fragment
void CacheMediaChunkBufferImpl::UpdateAccessPos(FragmentIterator& fragmentPos, ChunkIterator& chunkPos,
    uint64_t offsetChunk)
{
    if (chunkPos != fragmentPos->chunks.begin() && chunkPos == fragmentPos->chunks.end()) {
        auto preChunkPos = std::prev(chunkPos);
        if (((*preChunkPos)->offset + (*preChunkPos)->chunkSize) == offsetChunk) {
            fragmentPos->accessPos = chunkPos;
        } else {
            fragmentPos->accessPos = preChunkPos;
        }
    } else if ((*chunkPos)->offset == offsetChunk) {
            fragmentPos->accessPos = chunkPos;
    } else {
        fragmentPos->accessPos = std::prev(chunkPos);
    }
}

size_t CacheMediaChunkBufferImpl::Read(void* ptr, uint64_t offset, size_t readSize)
{
    std::lock_guard lock(mutex_);
    size_t hasReadSize = 0;
    uint8_t* dst = static_cast<uint8_t*>(ptr);
    uint64_t hasReadOffset = offset;
    size_t oneReadSize = ReadInner(dst, hasReadOffset, readSize);
    hasReadSize = oneReadSize;
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    while (hasReadSize < readSize && oneReadSize != 0) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        dst += oneReadSize;
        hasReadOffset += static_cast<uint64_t>(oneReadSize);
        oneReadSize = ReadInner(dst, hasReadOffset, readSize - hasReadSize);
        hasReadSize += oneReadSize;
    }
    return hasReadSize;
}

size_t CacheMediaChunkBufferImpl::ReadInner(void* ptr, uint64_t offset, size_t readSize)
{
    auto fragmentPos = GetOffsetFragmentCache(readPos_, offset, LeftBoundedRightOpenComp);
    if (readSize == 0 || fragmentPos == fragmentCacheBuffer_.end()) {
        return 0;
    }
    auto chunkPos = fragmentPos->accessPos;
    if (chunkPos == fragmentPos->chunks.end() ||
        offset < (*chunkPos)->offset ||
        offset > (*chunkPos)->offset + (*chunkPos)->dataLength) {
        chunkPos = GetOffsetChunkCache(fragmentPos->chunks, offset, LeftBoundedRightOpenComp);
    }

    uint8_t* dst = static_cast<uint8_t*>(ptr);
    uint64_t offsetChunk = offset;
    if (chunkPos != fragmentPos->chunks.end()) {
        uint64_t readOffset = offset > fragmentPos->offsetBegin ? offset - fragmentPos->offsetBegin : 0;
        uint64_t temp = readOffset > static_cast<uint64_t>(fragmentPos->accessLength) ?
                            readOffset - static_cast<uint64_t>(fragmentPos->accessLength) : 0;
        if (temp >= ACCESS_OFFSET_MAX_LENGTH) {
            chunkPos = SplitFragmentCacheBuffer(fragmentPos, offset, chunkPos);
        }
        size_t hasReadSize = 0;
        int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
        while (hasReadSize < readSize && chunkPos != fragmentPos->chunks.end()) {
            if (CheckLoopTimeout(loopStartTime)) {
                break;
            }
            auto chunkInfo = *chunkPos;
            uint64_t diff = offsetChunk > chunkInfo->offset ? offsetChunk - chunkInfo->offset : 0;
            if (offsetChunk < chunkInfo->offset || diff > chunkInfo->dataLength) {
                DumpAndCheckInner();
                return 0;
            }
            uint64_t readDiff = chunkInfo->dataLength > diff ? chunkInfo->dataLength - diff : 0;
            auto readOne = std::min(static_cast<size_t>(readDiff), readSize - hasReadSize);
            errno_t res = memcpy_s(dst + hasReadSize, readOne, (*chunkPos)->data + diff, readOne);
            FALSE_RETURN_V_MSG_E(res == EOK, 0, "memcpy_s data err");
            hasReadSize += readOne;
            offsetChunk += static_cast<uint64_t>(readOne);
            chunkPos++;
        }
        UpdateAccessPos(fragmentPos, chunkPos, offsetChunk);
        UpdateFragment(fragmentPos, hasReadSize, offsetChunk);
        return hasReadSize;
    }
    return 0;
}

void CacheMediaChunkBufferImpl::UpdateFragment(FragmentIterator& fragmentPos, size_t hasReadSize,
    uint64_t offsetChunk)
{
    uint64_t lengthDiff = offsetChunk > fragmentPos->offsetBegin ? offsetChunk - fragmentPos->offsetBegin : 0;
    fragmentPos->accessLength = static_cast<int64_t>(lengthDiff);
    fragmentPos->readTime = Clock::now();
    fragmentPos->totalReadSize += hasReadSize;
    totalReadSize_ += hasReadSize;
    readPos_ = fragmentPos;
    lruCache_.Refer(fragmentPos->offsetBegin, fragmentPos);
}

bool CacheMediaChunkBufferImpl::WriteInPlace(FragmentIterator& fragmentPos, uint8_t* ptr, uint64_t inOffset,
                                             size_t inWriteSize, size_t& outWriteSize)
{
    uint64_t offset = inOffset;
    size_t writeSize = inWriteSize;
    uint8_t* src = ptr;
    auto& chunkList = fragmentPos->chunks;
    outWriteSize = 0;
    ChunkIterator chunkPos = std::upper_bound(chunkList.begin(), chunkList.end(), offset,
        [](auto inputOffset, const CacheChunk* chunk) {
            return (inputOffset <= chunk->offset + chunk->dataLength);
        });
    if (chunkPos == chunkList.end()) {
        DumpInner(0);
        return false;
    }
    size_t writeSizeTmp = 0;
    auto chunkInfoTmp = *chunkPos;
    uint64_t accessLengthTmp = inOffset > writePos_->offsetBegin ? inOffset - writePos_->offsetBegin : 0;
    if (chunkInfoTmp->offset <= offset &&
        offset < chunkInfoTmp->offset + static_cast<uint64_t>(chunkInfoTmp->dataLength)) {
        size_t diff = static_cast<size_t>(offset > chunkInfoTmp->offset ? offset - chunkInfoTmp->offset : 0);
        size_t copyLen = static_cast<size_t>(chunkInfoTmp->dataLength - diff);
        copyLen = std::min(copyLen, writeSize);
        errno_t res = memcpy_s(chunkInfoTmp->data + diff, copyLen, src, copyLen);
        FALSE_RETURN_V_MSG_E(res == EOK, false, "memcpy_s data err");
        IncreaseStep(src, offset, writeSizeTmp, copyLen);
        if (writePos_->accessLength > static_cast<int64_t>(accessLengthTmp)) {
            writePos_->accessPos = chunkPos;
            writePos_->accessLength = static_cast<int64_t>(accessLengthTmp);
        }
    } else if (writePos_->accessLength > static_cast<int64_t>(accessLengthTmp)) {
        writePos_->accessPos = std::next(chunkPos);
        writePos_->accessLength = static_cast<int64_t>(accessLengthTmp);
    }
    ++chunkPos;
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    while (writeSizeTmp < writeSize && chunkPos != chunkList.end()) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        chunkInfoTmp = *chunkPos;
        auto copyLen = std::min(chunkInfoTmp->dataLength, (uint32_t)(writeSize - writeSizeTmp));
        errno_t res = memcpy_s(chunkInfoTmp->data, copyLen, src, copyLen);
        FALSE_RETURN_V_MSG_E(res == EOK, false, "memcpy_s data err");
        IncreaseStep(src, offset, writeSizeTmp, copyLen);
        ++chunkPos;
    }
    outWriteSize = writeSizeTmp;
    return true;
}

bool CacheMediaChunkBufferImpl::WriteMergerPre(uint64_t offset, size_t writeSize, FragmentIterator& nextFragmentPos)
{
    nextFragmentPos = std::next(writePos_);
    bool isLoop = true;
    while (isLoop) {
        if (nextFragmentPos == fragmentCacheBuffer_.end() ||
            offset + static_cast<uint64_t>(writeSize) < nextFragmentPos->offsetBegin) {
            nextFragmentPos = fragmentCacheBuffer_.end();
            isLoop = false;
            break;
        }
        if (offset + static_cast<uint64_t>(writeSize) <
                nextFragmentPos->offsetBegin + static_cast<uint64_t>(nextFragmentPos->dataLength)) {
            auto endPos = GetOffsetChunkCache(nextFragmentPos->chunks,
                                              offset + static_cast<uint64_t>(writeSize), LeftBoundedRightOpenComp);
            freeChunks_.splice(freeChunks_.end(), nextFragmentPos->chunks, nextFragmentPos->chunks.begin(), endPos);
            if (endPos == nextFragmentPos->chunks.end()) {
                nextFragmentPos = EraseFragmentCache(nextFragmentPos);
                DumpInner(0);
                return false;
            }
            auto &chunkInfo  = *endPos;
            uint64_t newOffset = offset + static_cast<uint64_t>(writeSize);
            uint64_t dataLength = static_cast<uint64_t>(chunkInfo->dataLength);
            uint64_t moveLen = std::max(chunkInfo->offset + dataLength, newOffset) - newOffset;
            auto mergeDataLen = chunkInfo->dataLength > moveLen ? chunkInfo->dataLength - moveLen : 0;
            errno_t res = memmove_s(chunkInfo->data, moveLen, chunkInfo->data + mergeDataLen, moveLen);
            FALSE_RETURN_V_MSG_E(res == EOK, false, "memmove_s data err");
            chunkInfo->offset = newOffset;
            chunkInfo->dataLength = static_cast<uint32_t>(moveLen);
            uint64_t lostLength = std::max(newOffset, nextFragmentPos->offsetBegin) - nextFragmentPos->offsetBegin;
            nextFragmentPos->dataLength -= static_cast<int64_t>(lostLength);
            lruCache_.Update(nextFragmentPos->offsetBegin, newOffset, nextFragmentPos);
            nextFragmentPos->offsetBegin = newOffset;
            nextFragmentPos->accessLength = 0;
            nextFragmentPos->accessPos = nextFragmentPos->chunks.end();
            isLoop = false;
            break;
        } else {
            freeChunks_.splice(freeChunks_.end(), nextFragmentPos->chunks);
            writePos_->totalReadSize += nextFragmentPos->totalReadSize;
            nextFragmentPos->totalReadSize = 0; // avoid total size sub, chunk num reduce.
            nextFragmentPos = EraseFragmentCache(nextFragmentPos);
        }
    }
    return true;
}

void CacheMediaChunkBufferImpl::WriteMergerPost(FragmentIterator& nextFragmentPos)
{
    if (nextFragmentPos == fragmentCacheBuffer_.end() || writePos_->chunks.empty() ||
        nextFragmentPos->chunks.empty()) {
        return;
    }
    auto preChunkInfo = writePos_->chunks.back();
    auto nextChunkInfo = nextFragmentPos->chunks.front();
    if (preChunkInfo->offset + preChunkInfo->dataLength != nextChunkInfo->offset) {
        DumpAndCheckInner();
        return;
    }
    writePos_->dataLength += nextFragmentPos->dataLength;
    writePos_->totalReadSize += nextFragmentPos->totalReadSize;
    nextFragmentPos->totalReadSize = 0; // avoid total size sub, chunk num reduce
    writePos_->chunks.splice(writePos_->chunks.end(), nextFragmentPos->chunks);
    EraseFragmentCache(nextFragmentPos);
}

size_t CacheMediaChunkBufferImpl::Write(void* ptr, uint64_t inOffset, size_t inWriteSize)
{
    std::lock_guard lock(mutex_);
    uint64_t offset = inOffset;
    size_t writeSize = inWriteSize;
    uint8_t* src = static_cast<uint8_t*>(ptr);
    size_t dupWriteSize = 0;

    auto fragmentPos = GetOffsetFragmentCache(writePos_, offset, BoundedIntervalComp);
    ChunkIterator chunkPos;
    if (fragmentPos != fragmentCacheBuffer_.end()) {
        auto& chunkList = fragmentPos->chunks;
        writePos_ = fragmentPos;
        if ((fragmentPos->offsetBegin + static_cast<uint64_t>(fragmentPos->dataLength)) != offset) {
            auto ret = WriteInPlace(fragmentPos, src, offset, writeSize, dupWriteSize);
            if (!ret || dupWriteSize >= writeSize) {
                return ret ? writeSize : dupWriteSize;
            }
            src += dupWriteSize;
            offset += dupWriteSize;
            writeSize -= dupWriteSize;
        }
        chunkPos = std::prev(chunkList.end());
    } else {
        if (freeChunks_.empty()) {
            MEDIA_LOG_D("no free chunk.");
        }
        MEDIA_LOG_D("not find fragment.");
        chunkPos = AddFragmentCacheBuffer(offset);
    }
    FragmentIterator nextFragmentPos = fragmentCacheBuffer_.end();
    auto success = WriteMergerPre(offset, writeSize, nextFragmentPos);
    if (!success) {
        return dupWriteSize;
    }
    auto writeSizeTmp = WriteChunk(*writePos_, chunkPos, src, offset, writeSize);
    if (writeSize != writeSizeTmp) {
        nextFragmentPos = fragmentCacheBuffer_.end();
    }
    WriteMergerPost(nextFragmentPos);
    return writeSizeTmp + dupWriteSize;
}

size_t CacheMediaChunkBufferHlsImpl::Write(void* ptr, uint64_t inOffset, size_t inWriteSize)
{
    std::lock_guard lock(mutex_);
    uint64_t offset = inOffset;
    size_t writeSize = inWriteSize;
    uint8_t* src = static_cast<uint8_t*>(ptr);
    size_t dupWriteSize = 0;

    auto fragmentPos = GetOffsetFragmentCache(writePos_, offset, BoundedIntervalComp);
    ChunkIterator chunkPos;
    if (fragmentPos != fragmentCacheBuffer_.end()) {
        auto& chunkList = fragmentPos->chunks;
        writePos_ = fragmentPos;
        if ((fragmentPos->offsetBegin + static_cast<uint64_t>(fragmentPos->dataLength)) != offset) {
            auto ret = WriteInPlace(fragmentPos, src, offset, writeSize, dupWriteSize);
            if (!ret || dupWriteSize >= writeSize) {
                return ret ? writeSize : dupWriteSize;
            }
            src += dupWriteSize;
            offset += dupWriteSize;
            writeSize -= dupWriteSize;
        }
        chunkPos = std::prev(chunkList.end());
    } else {
        if (freeChunks_.empty()) {
            return 0; // 只有hls可以return 0
        }
        MEDIA_LOG_D("not find fragment.");
        chunkPos = AddFragmentCacheBuffer(offset);
    }
    FragmentIterator nextFragmentPos = fragmentCacheBuffer_.end();
    auto success = WriteMergerPre(offset, writeSize, nextFragmentPos);
    if (!success) {
        return dupWriteSize;
    }
    auto writeSizeTmp = WriteChunk(*writePos_, chunkPos, src, offset, writeSize);
    if (writeSize != writeSizeTmp) {
        nextFragmentPos = fragmentCacheBuffer_.end();
    }
    WriteMergerPost(nextFragmentPos);
    return writeSizeTmp + dupWriteSize;
}

bool CacheMediaChunkBufferImpl::Seek(uint64_t offset)
{
    std::lock_guard lock(mutex_);
    auto readPos = GetOffsetFragmentCache(readPos_, offset, BoundedIntervalComp);
    if (readPos != fragmentCacheBuffer_.end()) {
        readPos_ = readPos;
        bool isSeekHit = false;
        auto chunkPos = GetOffsetChunkCache(readPos->chunks, offset, LeftBoundedRightOpenComp);
        if (chunkPos != readPos->chunks.end()) {
            auto readOffset = offset > readPos->offsetBegin ? offset - readPos->offsetBegin : 0;
            uint64_t diff = readOffset > static_cast<uint64_t>(readPos->accessLength) ?
                                readOffset - static_cast<uint64_t>(readPos->accessLength) : 0;
            if (diff >= ACCESS_OFFSET_MAX_LENGTH) {
                chunkPos = SplitFragmentCacheBuffer(readPos, offset, chunkPos);
            }

            if (chunkPos == readPos->chunks.end()) {
                return false;
            }
            lruCache_.Refer(readPos->offsetBegin, readPos);
            (*readPos).accessPos = chunkPos;
            auto tmpLength = offset > (*readPos).offsetBegin ? offset - (*readPos).offsetBegin : 0;
            (*readPos).accessLength = static_cast<int64_t>(tmpLength);
            readPos->readTime = Clock::now();
            isSeekHit = true;
        }
        ResetReadSizeAlloc();
        uint64_t newReadSizeInit = static_cast<uint64_t>(1 + initReadSizeFactor_ * static_cast<double>(totalReadSize_));
        readPos->totalReadSize += newReadSizeInit;
        totalReadSize_ += newReadSizeInit;
        return isSeekHit;
    }
    return false;
}

uint64_t CacheMediaChunkBufferImpl::GetBufferSize(uint64_t offset)
{
    std::lock_guard lock(mutex_);
    auto readPos = GetOffsetFragmentCache(readPos_, offset, LeftBoundedRightOpenComp);
    uint64_t bufferSize = 0;
    while (readPos != fragmentCacheBuffer_.end()) {
        uint64_t nextOffsetBegin = readPos->offsetBegin + static_cast<uint64_t>(readPos->dataLength);
        bufferSize = nextOffsetBegin > offset ? nextOffsetBegin - offset : 0;
        readPos++;
        if (readPos == fragmentCacheBuffer_.end() || nextOffsetBegin != readPos->offsetBegin) {
            break;
        }
    }
    return bufferSize;
}

void CacheMediaChunkBufferImpl::HandleFragmentPos(FragmentIterator& fragmentIter)
{
    uint64_t nextOffsetBegin = fragmentIter->offsetBegin + static_cast<uint64_t>(fragmentIter->dataLength);
    ++fragmentIter;
    while (fragmentIter != fragmentCacheBuffer_.end()) {
        if (nextOffsetBegin != fragmentIter->offsetBegin) {
            break;
        }
        nextOffsetBegin = fragmentIter->offsetBegin + static_cast<uint64_t>(fragmentIter->dataLength);
        ++fragmentIter;
    }
}

uint64_t CacheMediaChunkBufferImpl::GetNextBufferOffset(uint64_t offset)
{
    std::lock_guard lock(mutex_);
    auto fragmentIter = std::upper_bound(fragmentCacheBuffer_.begin(), fragmentCacheBuffer_.end(), offset,
        [](auto inputOffset, const FragmentCacheBuffer& fragment) {
            return (inputOffset < fragment.offsetBegin + fragment.dataLength);
        });
    if (fragmentIter != fragmentCacheBuffer_.end()) {
        if (LeftBoundedRightOpenComp(offset, fragmentIter->offsetBegin,
            fragmentIter->offsetBegin + fragmentIter->dataLength)) {
            HandleFragmentPos(fragmentIter);
        }
    }
    if (fragmentIter != fragmentCacheBuffer_.end()) {
        return fragmentIter->offsetBegin;
    }
    return 0;
}

FragmentIterator CacheMediaChunkBufferImpl::EraseFragmentCache(const FragmentIterator& iter)
{
    if (iter == readPos_) {
        readPos_ = fragmentCacheBuffer_.end();
    }
    if (iter == writePos_) {
        writePos_ = fragmentCacheBuffer_.end();
    }
    totalReadSize_ -= iter->totalReadSize;
    lruCache_.Delete(iter->offsetBegin);
    return fragmentCacheBuffer_.erase(iter);
}

inline size_t WriteOneChunkData(CacheChunk& chunkInfo, uint8_t* src, uint64_t offset, size_t writeSize)
{
    uint64_t copyBegin = offset > chunkInfo.offset ? offset - chunkInfo.offset : 0;
    if (copyBegin > chunkInfo.chunkSize) {
        return 0;
    }
    size_t writePerOne = static_cast<size_t>(chunkInfo.chunkSize - static_cast<size_t>(copyBegin));
    writePerOne = std::min(writePerOne, writeSize);
    errno_t res = memcpy_s(chunkInfo.data + copyBegin, writePerOne, src, writePerOne);
    FALSE_RETURN_V_MSG_E(res == EOK, 0, "memcpy_s data err");
    chunkInfo.dataLength = static_cast<uint32_t>(static_cast<size_t>(copyBegin) + writePerOne);
    return writePerOne;
}

inline CacheChunk* PopFreeCacheChunk(CacheChunkList& freeChunks, uint64_t offset)
{
    if (freeChunks.empty()) {
        return nullptr;
    }
    auto tmp = freeChunks.front();
    freeChunks.pop_front();
    InitChunkInfo(*tmp, offset);
    return tmp;
}

size_t CacheMediaChunkBufferImpl::WriteChunk(FragmentCacheBuffer& fragmentCacheBuffer, ChunkIterator& chunkPos,
                                             void* ptr, uint64_t offset, size_t writeSize)
{
    if (chunkPos == fragmentCacheBuffer.chunks.end()) {
        MEDIA_LOG_D("input valid.");
        return 0;
    }
    size_t writedTmp = 0;
    auto chunkInfo = *chunkPos;
    uint8_t* src =  static_cast<uint8_t*>(ptr);
    if (chunkInfo->chunkSize > chunkInfo->dataLength) {
        writedTmp += WriteOneChunkData(*chunkInfo, src, offset, writeSize);
        fragmentCacheBuffer.dataLength += static_cast<int64_t>(writedTmp);
    }
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    while (writedTmp < writeSize && writedTmp >= 0) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        auto chunkOffset = offset + static_cast<uint64_t>(writedTmp);
        auto freeChunk = GetFreeCacheChunk(chunkOffset);
        if (freeChunk == nullptr) {
            return writedTmp;
        }
        auto writePerOne = WriteOneChunkData(*freeChunk, src + writedTmp, chunkOffset, writeSize - writedTmp);
        fragmentCacheBuffer.chunks.push_back(freeChunk);
        writedTmp += writePerOne;
        fragmentCacheBuffer.dataLength += static_cast<int64_t>(writePerOne);

        if (fragmentCacheBuffer.accessPos == fragmentCacheBuffer.chunks.end()) {
            fragmentCacheBuffer.accessPos = std::prev(fragmentCacheBuffer.chunks.end());
        }
    }
    return writedTmp;
}

CacheChunk* CacheMediaChunkBufferImpl::UpdateFragmentCacheForDelHead(FragmentIterator& fragmentIter)
{
    FragmentCacheBuffer& fragment = *fragmentIter;
    if (fragment.chunks.empty()) {
        return nullptr;
    }
    auto cacheChunk = fragment.chunks.front();
    fragment.chunks.pop_front();

    auto oldOffsetBegin = fragment.offsetBegin;
    int64_t dataLength = static_cast<int64_t>(cacheChunk->dataLength);
    fragment.offsetBegin += static_cast<uint64_t>(dataLength);
    fragment.dataLength -= dataLength;
    if (fragment.accessLength > dataLength) {
        fragment.accessLength -= dataLength;
    } else {
        fragment.accessLength = 0;
    }
    lruCache_.Update(oldOffsetBegin, fragmentIter->offsetBegin, fragmentIter);
    return cacheChunk;
}

CacheChunk* UpdateFragmentCacheForDelTail(FragmentCacheBuffer& fragment)
{
    if (fragment.chunks.empty()) {
        return nullptr;
    }
    if (fragment.accessPos == std::prev(fragment.chunks.end())) {
        fragment.accessPos = fragment.chunks.end();
    }

    auto cacheChunk = fragment.chunks.back();
    fragment.chunks.pop_back();

    auto dataLength = cacheChunk->dataLength;
    if (fragment.accessLength > fragment.dataLength - static_cast<int64_t>(dataLength)) {
        fragment.accessLength = fragment.dataLength - static_cast<int64_t>(dataLength);
    }
    fragment.dataLength -= static_cast<int64_t>(dataLength);
    return cacheChunk;
}

bool CacheMediaChunkBufferImpl::CheckThresholdFragmentCacheBuffer(FragmentIterator& currWritePos)
{
    int64_t offset = -1;
    FragmentIterator fragmentIterator = fragmentCacheBuffer_.end();
    auto ret = lruCache_.GetLruNode(offset, fragmentIterator);
    if (!ret) {
        return false;
    }
    if (fragmentIterator == fragmentCacheBuffer_.end()) {
        return false;
    }
    if (currWritePos == fragmentIterator) {
        lruCache_.Refer(offset, currWritePos);
        ret = lruCache_.GetLruNode(offset, fragmentIterator);
        if (!ret) {
            return false;
        }
        if (fragmentIterator == fragmentCacheBuffer_.end()) {
            return false;
        }
    }
    if (fragmentIterator != fragmentCacheBuffer_.end()) {
        freeChunks_.splice(freeChunks_.end(), fragmentIterator->chunks);
        EraseFragmentCache(fragmentIterator);
    }
    return true;
}

/***
 * 总体策略：
 * 计算最大允许Fragment数，大于 FRAGMENT_MAX_NUM(4)则剔除最近为未读取的Fragment（不包含当前写的节点）
 * 新分配的节点固定分配 个chunk大小，通过公式计算，保证其能够下载；
 * 每个Fragment最大允许的Chunk数：（本Fragment读取字节（fragmentReadSize）/ 总读取字节（totalReadSize））* 总Chunk个数
 * 计算改Fragment最大允许的chunk个数
 *      如果超过，则删除对应已读chunk，如果没有已读chunk，还超则返回不允许继续写，返回失败；（说明该Fragment不能再写更多的内容）
 *      如果没有超过则从空闲队列中获取chunk，没有则
 *          for循环其他Fragment，计算每个Fragment的最大允许chunk个数：
 *              如果超过，则删除对应已读chunk
 *          如果还不够，则
 *              for循环其他Fragment，计算每个Fragment的最大允许chunk个数：
 *                  如果超过，则删除对应末尾未读chunk
 *
 * 如果还没有则返回失败
 *
 * 备注：是否一开始：优先从空闲队列中获取，没有则继续。
 */
void CacheMediaChunkBufferImpl::DeleteHasReadFragmentCacheBuffer(FragmentIterator& fragmentIter, size_t allowChunkNum)
{
    auto& fragmentCacheChunks = *fragmentIter;
    while (fragmentCacheChunks.chunks.size() >= allowChunkNum &&
        fragmentCacheChunks.accessLength > static_cast<int64_t>(static_cast<double>(fragmentCacheChunks.dataLength) *
        CACHE_RELEASE_FACTOR_DEFAULT / TO_PERCENT)) {
        if (fragmentCacheChunks.accessPos != fragmentCacheChunks.chunks.begin()) {
            auto tmp = UpdateFragmentCacheForDelHead(fragmentIter);
            if (tmp != nullptr) {
                freeChunks_.push_back(tmp);
            }
        } else {
            MEDIA_LOG_D("judge has read finish.");
            break;
        }
    }
}

void CacheMediaChunkBufferImpl::DeleteUnreadFragmentCacheBuffer(FragmentIterator& fragmentIter, size_t allowChunkNum)
{
    auto& fragmentCacheChunks = *fragmentIter;
    while (fragmentCacheChunks.chunks.size() > allowChunkNum) {
        if (!fragmentCacheChunks.chunks.empty()) {
            auto tmp = UpdateFragmentCacheForDelTail(fragmentCacheChunks);
            if (tmp != nullptr) {
                freeChunks_.push_back(tmp);
            }
        } else {
            break;
        }
    }
}

CacheChunk* CacheMediaChunkBufferImpl::GetFreeCacheChunk(uint64_t offset, bool checkAllowFailContinue)
{
    if (writePos_ == fragmentCacheBuffer_.end()) {
        return nullptr;
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
    }
    auto currWritePos = GetOffsetFragmentCache(writePos_, offset, BoundedIntervalComp);
    size_t allowChunkNum = 0;
    if (currWritePos != fragmentCacheBuffer_.end()) {
        allowChunkNum = CalcAllowMaxChunkNum(currWritePos->totalReadSize, currWritePos->offsetBegin);
        DeleteHasReadFragmentCacheBuffer(currWritePos, allowChunkNum);
        if (currWritePos->chunks.size() >= allowChunkNum && !checkAllowFailContinue) {
            return nullptr;
        }
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
    }
    for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end(); ++iter) {
        if (iter != currWritePos) {
            allowChunkNum = CalcAllowMaxChunkNum(iter->totalReadSize, iter->offsetBegin);
            DeleteHasReadFragmentCacheBuffer(iter, allowChunkNum);
        }
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
    }
    while (fragmentCacheBuffer_.size() > CACHE_FRAGMENT_MIN_NUM_DEFAULT) {
        auto result = CheckThresholdFragmentCacheBuffer(currWritePos);
        if (!freeChunks_.empty()) {
            return PopFreeCacheChunk(freeChunks_, offset);
        }
        if (!result) {
            break;
        }
    }
    for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end(); ++iter) {
        if (iter != currWritePos) {
            allowChunkNum = CalcAllowMaxChunkNum(iter->totalReadSize, iter->offsetBegin);
            DeleteUnreadFragmentCacheBuffer(iter, allowChunkNum);
        }
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
    }
    return nullptr;
}

CacheChunk* CacheMediaChunkBufferHlsImpl::GetFreeCacheChunk(uint64_t offset, bool checkAllowFailContinue)
{
    if (writePos_ == fragmentCacheBuffer_.end()) {
        return nullptr;
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
    }
    auto currWritePos = GetOffsetFragmentCache(writePos_, offset, BoundedIntervalComp);
    size_t allowChunkNum = 0;
    if (currWritePos != fragmentCacheBuffer_.end()) {
        allowChunkNum = CalcAllowMaxChunkNum(currWritePos->totalReadSize, currWritePos->offsetBegin);
        DeleteHasReadFragmentCacheBuffer(currWritePos, allowChunkNum);
        if (currWritePos->chunks.size() >= allowChunkNum && !checkAllowFailContinue) {
            MEDIA_LOG_D("allowChunkNum limit.");
            return nullptr;
        }
    } else {
        MEDIA_LOG_D("curr write is new fragment.");
    }
    MEDIA_LOG_D("clear other fragment has read chunk.");
    for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end(); ++iter) {
        if (iter != currWritePos) {
            allowChunkNum = CalcAllowMaxChunkNum(iter->totalReadSize, iter->offsetBegin);
            DeleteHasReadFragmentCacheBuffer(iter, allowChunkNum);
        }
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
    }
    return nullptr;
}

FragmentIterator CacheMediaChunkBufferImpl::GetFragmentIterator(FragmentIterator& currFragmentIter,
    uint64_t offset, ChunkIterator chunkPos, CacheChunk* splitHead, CacheChunk*& chunkInfo)
{
    auto newFragmentPos = fragmentCacheBuffer_.emplace(std::next(currFragmentIter), offset);
    if (splitHead == nullptr) {
        newFragmentPos->chunks.splice(newFragmentPos->chunks.end(), currFragmentIter->chunks, chunkPos,
            currFragmentIter->chunks.end());
    } else {
        splitHead->dataLength = 0;
        newFragmentPos->chunks.splice(newFragmentPos->chunks.end(), currFragmentIter->chunks, std::next(chunkPos),
            currFragmentIter->chunks.end());
        newFragmentPos->chunks.push_front(splitHead);
        splitHead->offset = offset;
        uint64_t diff = offset > chunkInfo->offset ? offset - chunkInfo->offset : 0;
        if (chunkInfo->dataLength >= diff) {
            splitHead->dataLength = chunkInfo->dataLength - static_cast<uint32_t>(diff);
            chunkInfo->dataLength = static_cast<uint32_t>(diff);
            memcpy_s(splitHead->data, splitHead->dataLength, chunkInfo->data + diff, splitHead->dataLength);
        }
    }
    newFragmentPos->offsetBegin = offset;
    uint64_t diff = offset > currFragmentIter->offsetBegin ? offset - currFragmentIter->offsetBegin : 0;
    newFragmentPos->dataLength = currFragmentIter->dataLength > static_cast<int64_t>(diff) ?
                                    currFragmentIter->dataLength - static_cast<int64_t>(diff) : 0;
    newFragmentPos->accessLength = 0;
    uint64_t newReadSizeInit = static_cast<uint64_t>(1 + initReadSizeFactor_ * static_cast<double>(totalReadSize_));
    newReadSizeInit = std::max(newReadSizeInit, currFragmentIter->totalReadSize);

    newFragmentPos->totalReadSize = newReadSizeInit;
    totalReadSize_ += newReadSizeInit;
    newFragmentPos->readTime = Clock::now();
    newFragmentPos->accessPos = newFragmentPos->chunks.begin();
    newFragmentPos->isSplit = currFragmentIter->isSplit;
    currFragmentIter->isSplit = true;
    currFragmentIter->dataLength = static_cast<int64_t>(offset > currFragmentIter->offsetBegin ?
                                        offset - currFragmentIter->offsetBegin : 0);
    return newFragmentPos;
}

ChunkIterator CacheMediaChunkBufferImpl::SplitFragmentCacheBuffer(FragmentIterator& currFragmentIter,
    uint64_t offset, ChunkIterator chunkPos)
{
    ResetReadSizeAlloc();
    auto& chunkInfo = *chunkPos;
    CacheChunk* splitHead = nullptr;
    if (offset != chunkInfo->offset) {
        splitHead = freeChunks_.empty() ? GetFreeCacheChunk(offset, true) : PopFreeCacheChunk(freeChunks_, offset);
        if (splitHead == nullptr) {
            return chunkPos;
        }
    }
    auto newFragmentPos = GetFragmentIterator(currFragmentIter, offset, chunkPos, splitHead, chunkInfo);
    currFragmentIter = newFragmentPos;
    if (fragmentCacheBuffer_.size() > CACHE_FRAGMENT_MAX_NUM_DEFAULT) {
        CheckThresholdFragmentCacheBuffer(currFragmentIter);
    }
    lruCache_.Refer(newFragmentPos->offsetBegin, newFragmentPos);
    return newFragmentPos->accessPos;
}

ChunkIterator CacheMediaChunkBufferHlsImpl::SplitFragmentCacheBuffer(FragmentIterator& currFragmentIter,
    uint64_t offset, ChunkIterator chunkPos)
{
    ResetReadSizeAlloc();
    auto& chunkInfo = *chunkPos;
    CacheChunk* splitHead = nullptr;
    if (offset != chunkInfo->offset) {
        splitHead = freeChunks_.empty() ? GetFreeCacheChunk(offset, true) : PopFreeCacheChunk(freeChunks_, offset);
        if (splitHead == nullptr) {
            return chunkPos;
        }
    }
    auto newFragmentPos = fragmentCacheBuffer_.emplace(std::next(currFragmentIter), offset);
    if (splitHead == nullptr) {
        newFragmentPos->chunks.splice(newFragmentPos->chunks.end(), currFragmentIter->chunks, chunkPos,
            currFragmentIter->chunks.end());
    } else {
        newFragmentPos->chunks.splice(newFragmentPos->chunks.end(), currFragmentIter->chunks, std::next(chunkPos),
            currFragmentIter->chunks.end());
        newFragmentPos->chunks.push_front(splitHead);
        splitHead->offset = offset;
        uint64_t diff = offset > chunkInfo->offset ? offset - chunkInfo->offset : 0;
        if (chunkInfo->dataLength >= diff) {
            splitHead->dataLength = chunkInfo->dataLength > static_cast<uint32_t>(diff) ?
                chunkInfo->dataLength - static_cast<uint32_t>(diff) : 0;
            chunkInfo->dataLength = static_cast<uint32_t>(diff);
            memcpy_s(splitHead->data, splitHead->dataLength, chunkInfo->data + diff, splitHead->dataLength);
        } else {
            splitHead->dataLength = 0; // It can't happen. us_asan can check.
        }
    }
    newFragmentPos->offsetBegin = offset;
    uint64_t diff = offset > currFragmentIter->offsetBegin ? offset - currFragmentIter->offsetBegin : 0;
    newFragmentPos->dataLength = currFragmentIter->dataLength > static_cast<int64_t>(diff) ?
                                    currFragmentIter->dataLength - static_cast<int64_t>(diff) : 0;
    newFragmentPos->accessLength = 0;
    uint64_t newReadSizeInit = static_cast<uint64_t>(1 + initReadSizeFactor_ * static_cast<double>(totalReadSize_));
    if (currFragmentIter->totalReadSize > newReadSizeInit) {
        newReadSizeInit = currFragmentIter->totalReadSize;
    }
    newFragmentPos->totalReadSize = newReadSizeInit;
    totalReadSize_ += newReadSizeInit;
    newFragmentPos->readTime = Clock::now();
    newFragmentPos->accessPos = newFragmentPos->chunks.begin();
    currFragmentIter->dataLength = static_cast<int64_t>(offset > currFragmentIter->offsetBegin ?
                                        offset - currFragmentIter->offsetBegin : 0);
    currFragmentIter = newFragmentPos;
    lruCache_.Refer(newFragmentPos->offsetBegin, newFragmentPos);
    return newFragmentPos->accessPos;
}

ChunkIterator CacheMediaChunkBufferImpl::AddFragmentCacheBuffer(uint64_t offset)
{
    size_t fragmentThreshold = CACHE_FRAGMENT_MAX_NUM_DEFAULT;
    if (isLargeOffsetSpan_) {
        fragmentThreshold = CACHE_FRAGMENT_MAX_NUM_LARGE;
    }
    if (fragmentCacheBuffer_.size() >= fragmentThreshold) {
        auto fragmentIterTmp = fragmentCacheBuffer_.end();
        CheckThresholdFragmentCacheBuffer(fragmentIterTmp);
    }
    ResetReadSizeAlloc();
    auto fragmentInsertPos = std::upper_bound(fragmentCacheBuffer_.begin(), fragmentCacheBuffer_.end(), offset,
        [](auto mediaOffset, const FragmentCacheBuffer& fragment) {
            if (mediaOffset <= fragment.offsetBegin + fragment.dataLength) {
                return true;
            }
            return false;
        });
    auto newFragmentPos = fragmentCacheBuffer_.emplace(fragmentInsertPos, offset);
    uint64_t newReadSizeInit = static_cast<uint64_t>(1 + initReadSizeFactor_ * static_cast<double>(totalReadSize_));
    totalReadSize_ += newReadSizeInit;
    newFragmentPos->totalReadSize = newReadSizeInit;
    writePos_ = newFragmentPos;
    writePos_->accessPos = writePos_->chunks.end();
    lruCache_.Refer(newFragmentPos->offsetBegin, newFragmentPos);
    auto freeChunk = GetFreeCacheChunk(offset);
    if (freeChunk == nullptr) {
        MEDIA_LOG_D("get free cache chunk fail.");
        return writePos_->chunks.end();
    }
    writePos_->accessPos = newFragmentPos->chunks.emplace(newFragmentPos->chunks.end(), freeChunk);
    return writePos_->accessPos;
}

ChunkIterator CacheMediaChunkBufferHlsImpl::AddFragmentCacheBuffer(uint64_t offset)
{
    ResetReadSizeAlloc();
    auto fragmentInsertPos = std::upper_bound(fragmentCacheBuffer_.begin(), fragmentCacheBuffer_.end(), offset,
        [](auto mediaOffset, const FragmentCacheBuffer& fragment) {
            if (mediaOffset <= fragment.offsetBegin + fragment.dataLength) {
                return true;
            }
            return false;
        });
    auto newFragmentPos = fragmentCacheBuffer_.emplace(fragmentInsertPos, offset);
    uint64_t newReadSizeInit = static_cast<uint64_t>(1 + initReadSizeFactor_ * static_cast<double>(totalReadSize_));
    totalReadSize_ += newReadSizeInit;
    newFragmentPos->totalReadSize = newReadSizeInit;
    writePos_ = newFragmentPos;
    writePos_->accessPos = writePos_->chunks.end();
    lruCache_.Refer(newFragmentPos->offsetBegin, newFragmentPos);
    auto freeChunk = GetFreeCacheChunk(offset);
    if (freeChunk == nullptr) {
        MEDIA_LOG_D("get free cache chunk fail.");
        return writePos_->chunks.end();
    }
    writePos_->accessPos = newFragmentPos->chunks.emplace(newFragmentPos->chunks.end(), freeChunk);
    return writePos_->accessPos;
}

void CacheMediaChunkBufferImpl::ResetReadSizeAlloc()
{
    size_t chunkNum = chunkMaxNum_ + 1 >= freeChunks_.size() ?
                        chunkMaxNum_ + 1 - freeChunks_.size() : 0;
    if (totalReadSize_ > static_cast<size_t>(UP_LIMIT_MAX_TOTAL_READ_SIZE) && chunkNum > 0) {
        size_t preChunkSize = static_cast<size_t>(MAX_TOTAL_READ_SIZE - 1) / chunkNum;
        for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end(); ++iter) {
            iter->totalReadSize = preChunkSize * iter->chunks.size();
        }
        totalReadSize_ = preChunkSize * chunkNum;
    }
}

void CacheMediaChunkBufferImpl::Dump(uint64_t param)
{
    std::lock_guard lock(mutex_);
    DumpInner(param);
}

void CacheMediaChunkBufferImpl::DumpInner(uint64_t param)
{
    (void)param;
    MEDIA_LOG_D("cacheBuff total buffer size : " PUBLIC_LOG_U64, totalBuffSize_);
    MEDIA_LOG_D("cacheBuff total chunk size  : " PUBLIC_LOG_U32, chunkSize_);
    MEDIA_LOG_D("cacheBuff total chunk num   : " PUBLIC_LOG_U32, chunkMaxNum_);
    MEDIA_LOG_D("cacheBuff total read size   : " PUBLIC_LOG_U64, totalReadSize_);
    MEDIA_LOG_D("cacheBuff read size factor  : " PUBLIC_LOG_F, initReadSizeFactor_);
    MEDIA_LOG_D("cacheBuff free chunk num    : " PUBLIC_LOG_ZU, freeChunks_.size());
    MEDIA_LOG_D("cacheBuff fragment num      : " PUBLIC_LOG_ZU, fragmentCacheBuffer_.size());
    for (auto const & fragment : fragmentCacheBuffer_) {
        MEDIA_LOG_D("cacheBuff - fragment offset : " PUBLIC_LOG_U64, fragment.offsetBegin);
        MEDIA_LOG_D("cacheBuff   fragment length : " PUBLIC_LOG_D64, fragment.dataLength);
        MEDIA_LOG_D("cacheBuff   chunk num       : " PUBLIC_LOG_ZU, fragment.chunks.size());
        MEDIA_LOG_D("cacheBuff   access length   : " PUBLIC_LOG_U64, fragment.accessLength);
        MEDIA_LOG_D("cacheBuff   read size       : " PUBLIC_LOG_U64, fragment.totalReadSize);
        if (fragment.accessPos != fragment.chunks.end()) {
            auto &chunkInfo = *fragment.accessPos;
            MEDIA_LOG_D("cacheBuff   access offset: " PUBLIC_LOG_D64 ", len: " PUBLIC_LOG_U32,
                chunkInfo->offest, chunkInfo->dataLength);
        } else {
            MEDIA_LOG_D("cacheBuff   access ended");
        }
        if (!fragment.chunks.empty()) {
            auto &chunkInfo = fragment.chunks.back();
            MEDIA_LOG_D("cacheBuff   last chunk offset: " PUBLIC_LOG_D64 ", len: " PUBLIC_LOG_U32,
                chunkInfo->offset, chunkInfo->dataLength);
        }
        MEDIA_LOG_D("cacheBuff ");
    }
}

bool CacheMediaChunkBufferImpl::CheckLoopTimeout(int64_t loopStartTime)
{
    int64_t now = loopInterruptClock_.ElapsedSeconds();
    int64_t loopDuration = now > loopStartTime ? now - loopStartTime : 0;
    bool isLoopTimeout = loopDuration > LOOP_TIMEOUT;
    if (isLoopTimeout) {
        MEDIA_LOG_E("loop timeout.");
    }
    return isLoopTimeout;
}

bool CacheMediaChunkBufferImpl::Check()
{
    std::lock_guard lock(mutex_);
    return CheckInner();
}

void CacheMediaChunkBufferImpl::Clear()
{
    std::lock_guard lock(mutex_);
    auto iter = fragmentCacheBuffer_.begin();
    while (iter != fragmentCacheBuffer_.end()) {
        freeChunks_.splice(freeChunks_.end(), iter->chunks);
        iter = EraseFragmentCache(iter);
    }
    lruCache_.Reset();
    totalReadSize_ = 0;
}

uint64_t CacheMediaChunkBufferImpl::GetFreeSize()
{
    std::lock_guard lock(mutex_);
    uint64_t totalFreeSize = totalBuffSize_;
    for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end(); iter++) {
        uint64_t fragmentDataLen = static_cast<uint64_t>(iter->dataLength);
        totalFreeSize = totalFreeSize > fragmentDataLen ? totalFreeSize - fragmentDataLen : 0;
    }
    return totalFreeSize;
}

// Release all fragments before the offset.
bool CacheMediaChunkBufferImpl::ClearChunksOfFragment(uint64_t offset)
{
    std::lock_guard lock(mutex_);
    bool res = false;
    auto fragmentPos = GetOffsetFragmentCache(readPos_, offset, LeftBoundedRightOpenComp);
    if (fragmentPos == fragmentCacheBuffer_.end()) {
        return false;
    }
    auto& fragment = *fragmentPos;
    if (fragment.chunks.empty()) {
        return false;
    }
    uint32_t chunkSize = fragment.chunks.size();
    for (uint32_t i = 0; i < chunkSize; ++i) {
        auto chunkIter = fragment.chunks.front();
        if (fragmentPos->accessPos == fragmentPos->chunks.end() || chunkIter == nullptr ||
            chunkIter->offset + chunkIter->dataLength >= offset) {
            break;
        }

        auto chunkPos = fragmentPos->accessPos;
        if ((*chunkPos) != nullptr && chunkIter->offset >= (*chunkPos)->offset) { // Update accessPos of fragment
            chunkPos = GetOffsetChunkCache(fragmentPos->chunks, chunkIter->offset + chunkIter->dataLength,
                LeftBoundedRightOpenComp);
            (*fragmentPos).accessPos = chunkPos;
        }

        MEDIA_LOG_D("ClearChunksOfFragment clear chunk, offsetBegin: " PUBLIC_LOG_U64 " offsetEnd " PUBLIC_LOG_U64,
            chunkIter->offset, chunkIter->offset + chunkIter->dataLength);
        auto tmp = UpdateFragmentCacheForDelHead(fragmentPos);
        if (tmp != nullptr) {
            res = true;
            freeChunks_.push_back(tmp);
        }
    }
    return res;
}

// Release all chunks before the offset in the fragment to which the specified offset belongs.
bool CacheMediaChunkBufferImpl::ClearFragmentBeforeOffset(uint64_t offset)
{
    std::lock_guard lock(mutex_);
    bool res = false;
    for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end();) {
        if (iter->offsetBegin >= offset) {
            break;
        }
        if (iter->offsetBegin + static_cast<uint64_t>(iter->dataLength) <= offset) {
            MEDIA_LOG_D("ClearFragmentBeforeOffset clear fragment, offsetBegin: " PUBLIC_LOG_U64 " offsetEnd "
                PUBLIC_LOG_U64, iter->offsetBegin, iter->offsetBegin + iter->dataLength);
            freeChunks_.splice(freeChunks_.end(), iter->chunks);
            iter = EraseFragmentCache(iter);
            res = true;
            continue;
        }
        iter++;
    }
    return res;
}

// Release all chunks of read fragment between minReadOffset and maxReadOffset.
bool CacheMediaChunkBufferImpl::ClearMiddleReadFragment(uint64_t minOffset, uint64_t maxOffset)
{
    std::lock_guard lock(mutex_);
    bool res = false;
    for (auto iter = fragmentCacheBuffer_.begin(); iter != fragmentCacheBuffer_.end(); iter++) {
        if (iter->offsetBegin + static_cast<uint64_t>(iter->dataLength) < minOffset) {
            continue;
        }
        if (iter->offsetBegin > maxOffset) {
            break;
        }
        if (iter->accessLength <= chunkSize_) {
            continue;
        }
        MEDIA_LOG_D("ClearMiddleReadFragment, minOffset: " PUBLIC_LOG_U64 " maxOffset: "
            PUBLIC_LOG_U64 " offsetBegin: " PUBLIC_LOG_U64 " dataLength: " PUBLIC_LOG_D64 " accessLength "
            PUBLIC_LOG_D64, minOffset, maxOffset, iter->offsetBegin,  iter->dataLength, iter->accessLength);
        auto& fragment = *iter;
        uint32_t chunksSize = fragment.chunks.size();
        for (uint32_t i = 0; i < chunksSize; ++i) {
            auto chunkIter = fragment.chunks.front();
            if (chunkIter->dataLength >= iter->accessLength ||
                (chunkIter->offset + chunkIter->dataLength >= maxOffset &&
                chunkIter->offset <= minOffset)) {
                break;
            }
            auto tmp = UpdateFragmentCacheForDelHead(iter);
            if (tmp != nullptr) {
                freeChunks_.push_back(tmp);
            }
        }
    }
    return res;
}

bool CacheMediaChunkBufferImpl::IsReadSplit(uint64_t offset)
{
    std::lock_guard lock(mutex_);
    auto readPos = GetOffsetFragmentCache(readPos_, offset, LeftBoundedRightOpenComp);
    if (readPos != fragmentCacheBuffer_.end()) {
        return readPos->isSplit;
    }
    return false;
}

void CacheMediaChunkBufferImpl::SetIsLargeOffsetSpan(bool isLargeOffsetSpan)
{
    isLargeOffsetSpan_ = isLargeOffsetSpan;
}

bool CacheMediaChunkBufferImpl::DumpAndCheckInner()
{
    DumpInner(0);
    return CheckInner();
}

void CacheMediaChunkBufferImpl::CheckFragment(const FragmentCacheBuffer& fragment, bool& checkSuccess)
{
    if (fragment.accessPos != fragment.chunks.end()) {
        auto& accessChunk = *fragment.accessPos;
        auto accessLength = accessChunk->offset > fragment.offsetBegin ?
            accessChunk->offset - fragment.offsetBegin : 0;
        if (fragment.accessLength < accessLength ||
            fragment.accessLength >
            (static_cast<int64_t>(accessLength) + static_cast<int64_t>(accessChunk->dataLength))) {
            checkSuccess = false;
        }
    }
}

bool CacheMediaChunkBufferImpl::CheckInner()
{
    uint64_t chunkNum = 0;
    uint64_t totalReadSize = 0;
    bool checkSuccess = true;
    chunkNum = freeChunks_.size();
    for (auto const& fragment : fragmentCacheBuffer_) {
        int64_t dataLength = 0;
        chunkNum += fragment.chunks.size();
        totalReadSize += fragment.totalReadSize;

        auto prev = fragment.chunks.begin();
        auto next = fragment.chunks.end();
        if (!fragment.chunks.empty()) {
            dataLength += static_cast<int64_t>((*prev)->dataLength);
            next = std::next(prev);
            if ((*prev)->offset != fragment.offsetBegin) {
                checkSuccess = false;
            }
        }
        while (next != fragment.chunks.end()) {
            auto &chunkPrev = *prev;
            auto &chunkNext = *next;
            dataLength += static_cast<int64_t>(chunkNext->dataLength);
            if (chunkPrev->offset + chunkPrev->dataLength != chunkNext->offset) {
                checkSuccess = false;
            }
            ++next;
            ++prev;
        }
        if (dataLength != fragment.dataLength) {
            checkSuccess = false;
        }
        CheckFragment(fragment, checkSuccess);
    }
    if (chunkNum != chunkMaxNum_ + 1) {
        checkSuccess = false;
    }

    if (totalReadSize != totalReadSize_) {
        checkSuccess = false;
    }
    return checkSuccess;
}


CacheMediaChunkBuffer::CacheMediaChunkBuffer()
{
    MEDIA_LOG_D("enter");
    impl_ = std::make_unique<CacheMediaChunkBufferImpl>();
};

CacheMediaChunkBuffer::~CacheMediaChunkBuffer()
{
    MEDIA_LOG_D("exit");
}

bool CacheMediaChunkBuffer::Init(uint64_t totalBuffSize, uint32_t chunkSize)
{
    return impl_->Init(totalBuffSize, chunkSize);
}

size_t CacheMediaChunkBuffer::Read(void* ptr, uint64_t offset, size_t readSize)
{
    return impl_->Read(ptr, offset, readSize);
}

size_t CacheMediaChunkBuffer::Write(void* ptr, uint64_t offset, size_t writeSize)
{
    return impl_->Write(ptr, offset, writeSize);
}

bool CacheMediaChunkBuffer::Seek(uint64_t offset)
{
    return impl_->Seek(offset);
}

uint64_t CacheMediaChunkBuffer::GetBufferSize(uint64_t offset)
{
    return impl_->GetBufferSize(offset);
}

uint64_t CacheMediaChunkBuffer::GetNextBufferOffset(uint64_t offset)
{
    return impl_->GetNextBufferOffset(offset);
}

void CacheMediaChunkBuffer::Clear()
{
    return impl_->Clear();
}

uint64_t CacheMediaChunkBuffer::GetFreeSize()
{
    return impl_->GetFreeSize();
}

bool CacheMediaChunkBuffer::ClearFragmentBeforeOffset(uint64_t offset)
{
    return impl_->ClearFragmentBeforeOffset(offset);
}

bool CacheMediaChunkBuffer::ClearChunksOfFragment(uint64_t offset)
{
    return impl_->ClearChunksOfFragment(offset);
}

bool CacheMediaChunkBuffer::ClearMiddleReadFragment(uint64_t minOffset, uint64_t maxOffset)
{
    return impl_->ClearMiddleReadFragment(minOffset, maxOffset);
}

bool CacheMediaChunkBuffer::IsReadSplit(uint64_t offset)
{
    return impl_->IsReadSplit(offset);
}

void CacheMediaChunkBuffer::SetIsLargeOffsetSpan(bool isLargeOffsetSpan)
{
    return impl_->SetIsLargeOffsetSpan(isLargeOffsetSpan);
}

void CacheMediaChunkBuffer::SetReadBlocking(bool isReadBlockingAllowed)
{
    (void)isReadBlockingAllowed;
}

void CacheMediaChunkBuffer::Dump(uint64_t param)
{
    return impl_->Dump(param);
}

bool CacheMediaChunkBuffer::Check()
{
    return impl_->Check();
}
}
}