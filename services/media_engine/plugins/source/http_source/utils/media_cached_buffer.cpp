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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {

constexpr size_t CACHE_FRAGMENT_MAX_NUM_DEFAULT = 10; // Minimum number of fragment nodes
constexpr size_t CACHE_FRAGMENT_MIN_NUM_DEFAULT = 3; // Maximum number of fragment nodes
constexpr double NEW_FRAGMENT_INIT_CHUNK_NUM = 128.0; // Restricting the cache size of seek operation, 128 = 2MB
constexpr double NEW_FRAGMENT_NIT_DEFAULT_DENOMINATOR = 0.25;
constexpr double CACHE_RELEASE_FACTOR_DEFAULT = 10;
constexpr double TO_PERCENT = 100;
constexpr int64_t MAX_TOTAL_READ_SIZE = 2000000;
constexpr int64_t UP_LIMIT_MAX_TOTAL_READ_SIZE = 3000000;
constexpr int64_t ACCESS_OFFSET_MAX_LENGTH = 2 * 1024;

inline constexpr bool BoundedIntervalComp(int64_t mid, int64_t start, int64_t end)
{
    return (start <= mid && mid <= end);
}

inline constexpr bool LeftBoundedRightOpenComp(int64_t mid, int64_t start, int64_t end)
{
    return (start <= mid && mid < end);
}

inline void IncreaseStep(uint8_t*& src, int64_t& offset, size_t& writeSize, size_t step)
{
    src += step;
    offset += static_cast<int64_t>(step);
    writeSize += step;
}

inline void InitChunkInfo(CacheChunk& chunkInfo, int64_t offset)
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
    lruCache_.ReCacheSize(CACHE_FRAGMENT_MAX_NUM_DEFAULT);

    if (totalBuffSize == 0 || chunkSize == 0 || totalBuffSize < chunkSize) {
        return false;
    }

    double newFragmentInitChunkNum  = NEW_FRAGMENT_INIT_CHUNK_NUM;
    int64_t chunkNum = totalBuffSize + chunkSize >= 1 ?
                       static_cast<int64_t>((totalBuffSize + chunkSize - 1) / chunkSize) + 1 : 1; // 1
    if ((chunkNum - static_cast<int64_t>(newFragmentInitChunkNum)) < 0) {
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
    FALSE_RETURN_V_MSG_E(static_cast<int64_t>(sizePerChunk) * chunkNum > 0, false,
        "Invalid sizePerChunk and chunkNum.");
    bufferAddr_ = static_cast<uint8_t*>(malloc(sizePerChunk * chunkNum));
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
    return true;
}

void CacheMediaChunkBufferImpl::UpdateAccessPos(FragmentIterator& fragmentPos, ChunkIterator& chunkPos,
    int64_t offsetChunk)
{
    if (chunkPos == fragmentPos->chunks.end()) {
        auto preChunkPos = std::prev(chunkPos);
        if (((*preChunkPos)->offset + static_cast<int64_t>((*preChunkPos)->chunkSize)) == offsetChunk) {
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

size_t CacheMediaChunkBufferImpl::Read(void* ptr, int64_t offset, size_t readSize)
{
    std::lock_guard lock(mutex_);
    size_t hasReadSize = 0;
    uint8_t* dst = static_cast<uint8_t*>(ptr);
    int64_t hasReadOffset = offset;
    size_t oneReadSize = ReadInner(dst, hasReadOffset, readSize);
    hasReadSize = oneReadSize;
    while (hasReadSize < readSize && oneReadSize != 0) {
        dst += oneReadSize;
        hasReadOffset += static_cast<int64_t>(oneReadSize);
        oneReadSize = ReadInner(dst, hasReadOffset, readSize - hasReadSize);
        hasReadSize += oneReadSize;
    }
    return hasReadSize;
}

size_t CacheMediaChunkBufferImpl::ReadInner(void* ptr, int64_t offset, size_t readSize)
{
    auto fragmentPos = GetOffsetFragmentCache(readPos_, offset, LeftBoundedRightOpenComp);
    if (readSize == 0 || fragmentPos == fragmentCacheBuffer_.end()) {
        return 0;
    }
    auto chunkPos = fragmentPos->accessPos;
    if (chunkPos == fragmentPos->chunks.end() ||
        offset < (*chunkPos)->offset ||
        offset > (*chunkPos)->offset + static_cast<int64_t>((*chunkPos)->dataLength)) {
        chunkPos = GetOffsetChunkCache(fragmentPos->chunks, offset, LeftBoundedRightOpenComp);
    }

    uint8_t* dst = static_cast<uint8_t*>(ptr);
    int64_t offsetChunk = offset;
    if (chunkPos != fragmentPos->chunks.end()) {
        auto readOffset = offset - fragmentPos->offsetBegin;
        if ((readOffset - fragmentPos->accessLength) >= ACCESS_OFFSET_MAX_LENGTH) {
            chunkPos = SplitFragmentCacheBuffer(fragmentPos, offset, chunkPos);
        }
        size_t hasReadSize = 0;
        while (hasReadSize < readSize && chunkPos != fragmentPos->chunks.end()) {
            auto chunkInfo = *chunkPos;
            auto diff = offsetChunk - chunkInfo->offset;
            if (offsetChunk < chunkInfo->offset || diff > chunkInfo->dataLength) {
                DumpAndCheckInner();
                return 0;
            }
            auto readOne = std::min((size_t)(chunkInfo->dataLength - diff), readSize - hasReadSize);
            errno_t res = memcpy_s(dst + hasReadSize, readOne, (*chunkPos)->data + diff, readOne);
            FALSE_RETURN_V_MSG_E(res == EOK, 0, "memcpy_s data err");
            hasReadSize += readOne;
            offsetChunk += static_cast<int64_t>(readOne);
            chunkPos++;
        }
        UpdateAccessPos(fragmentPos, chunkPos, offsetChunk);
        fragmentPos->accessLength = offsetChunk - fragmentPos->offsetBegin;
        fragmentPos->readTime = Clock::now();
        fragmentPos->totalReadSize += hasReadSize;
        totalReadSize_ += hasReadSize;
        readPos_ = fragmentPos;
        lruCache_.Refer(fragmentPos->offsetBegin, fragmentPos);
        return hasReadSize;
    }
    return 0;
}

bool CacheMediaChunkBufferImpl::WriteInPlace(FragmentIterator& fragmentPos, uint8_t* ptr, int64_t inOffset,
                                             size_t inWriteSize, size_t& outWriteSize)
{
    int64_t offset = inOffset;
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
    auto accessLengthTmp = inOffset - writePos_->offsetBegin;
    if (chunkInfoTmp->offset <= offset &&
        offset < chunkInfoTmp->offset + static_cast<int64_t>(chunkInfoTmp->dataLength)) {
        size_t diff = static_cast<size_t>(offset - chunkInfoTmp->offset);
        size_t copyLen = static_cast<size_t>(chunkInfoTmp->dataLength - diff);
        copyLen = std::min(copyLen, writeSize);
        errno_t res = memcpy_s(chunkInfoTmp->data + diff, copyLen, src, copyLen);
        FALSE_RETURN_V_MSG_E(res == EOK, false, "memcpy_s data err");
        IncreaseStep(src, offset, writeSizeTmp, copyLen);
        if (writePos_->accessLength > accessLengthTmp) {
            writePos_->accessPos = chunkPos;
            writePos_->accessLength = accessLengthTmp;
        }
    } else if (writePos_->accessLength > accessLengthTmp) {
        writePos_->accessPos = std::next(chunkPos);
        writePos_->accessLength = accessLengthTmp;
    }
    ++chunkPos;
    while (writeSizeTmp < writeSize && chunkPos != chunkList.end()) {
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

bool CacheMediaChunkBufferImpl::WriteMergerPre(int64_t offset, size_t writeSize, FragmentIterator& nextFragmentPos)
{
    nextFragmentPos = std::next(writePos_);
    bool isLoop = true;
    while (isLoop) {
        if (nextFragmentPos == fragmentCacheBuffer_.end() ||
            offset + static_cast<int64_t>(writeSize) < nextFragmentPos->offsetBegin) {
            nextFragmentPos = fragmentCacheBuffer_.end();
            isLoop = false;
            break;
        }
        if (offset + static_cast<int64_t>(writeSize) < nextFragmentPos->offsetBegin + nextFragmentPos->dataLength) {
            auto endPos = GetOffsetChunkCache(nextFragmentPos->chunks,
                                              offset + static_cast<int64_t>(writeSize), LeftBoundedRightOpenComp);
            freeChunks_.splice(freeChunks_.end(), nextFragmentPos->chunks, nextFragmentPos->chunks.begin(), endPos);
            if (endPos == nextFragmentPos->chunks.end()) {
                nextFragmentPos = EraseFragmentCache(nextFragmentPos);
                DumpInner(0);
                return false;
            }
            auto &chunkInfo  = *endPos;
            int64_t newOffset = offset + static_cast<int64_t>(writeSize);
            int64_t dataLength = static_cast<int64_t>(chunkInfo->dataLength);
            size_t moveLen = static_cast<size_t>(chunkInfo->offset + dataLength - newOffset);
            auto mergeDataLen = chunkInfo->dataLength - static_cast<uint64_t>(moveLen);
            errno_t res = memmove_s(chunkInfo->data, moveLen, chunkInfo->data + mergeDataLen, moveLen);
            FALSE_RETURN_V_MSG_E(res == EOK, false, "memmove_s data err");
            chunkInfo->offset = newOffset;
            chunkInfo->dataLength = static_cast<uint32_t>(moveLen);
            auto lostLength = newOffset - nextFragmentPos->offsetBegin;
            nextFragmentPos->dataLength -= lostLength;
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
    if (preChunkInfo->offset + static_cast<int64_t>(preChunkInfo->dataLength) != nextChunkInfo->offset) {
        DumpAndCheckInner();
        return;
    }
    writePos_->dataLength += nextFragmentPos->dataLength;
    writePos_->totalReadSize += nextFragmentPos->totalReadSize;
    nextFragmentPos->totalReadSize = 0; // avoid total size sub, chunk num reduce
    writePos_->chunks.splice(writePos_->chunks.end(), nextFragmentPos->chunks);
    EraseFragmentCache(nextFragmentPos);
}

size_t CacheMediaChunkBufferImpl::Write(void* ptr, int64_t inOffset, size_t inWriteSize)
{
    std::lock_guard lock(mutex_);
    int64_t offset = inOffset;
    size_t writeSize = inWriteSize;
    uint8_t* src = static_cast<uint8_t*>(ptr);
    size_t dupWriteSize = 0;

    auto fragmentPos = GetOffsetFragmentCache(writePos_, offset, BoundedIntervalComp);
    ChunkIterator chunkPos;
    if (fragmentPos != fragmentCacheBuffer_.end()) {
        auto& chunkList = fragmentPos->chunks;
        writePos_ = fragmentPos;
        if ((fragmentPos->offsetBegin + fragmentPos->dataLength) != offset) {
            auto ret = WriteInPlace(fragmentPos, src, offset, writeSize, dupWriteSize);
            if (!ret || dupWriteSize >= writeSize) {
                return writeSize;
            }
            src += dupWriteSize;
            offset += static_cast<int64_t>(dupWriteSize);
            writeSize -= dupWriteSize;
        }
        chunkPos = std::prev(chunkList.end());
    } else {
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

bool CacheMediaChunkBufferImpl::Seek(int64_t offset)
{
    std::lock_guard lock(mutex_);
    auto readPos = GetOffsetFragmentCache(readPos_, offset, BoundedIntervalComp);
    if (readPos != fragmentCacheBuffer_.end()) {
        readPos_ = readPos;
        bool isSeekHit = false;
        auto chunkPos = GetOffsetChunkCache(readPos->chunks, offset, LeftBoundedRightOpenComp);
        if (chunkPos != readPos->chunks.end()) {
            auto readOffset = offset - readPos->offsetBegin;
            if ((readOffset - readPos->accessLength) >= ACCESS_OFFSET_MAX_LENGTH) {
                chunkPos = SplitFragmentCacheBuffer(readPos, offset, chunkPos);
            }

            if (chunkPos == readPos->chunks.end()) {
                return false;
            }
            lruCache_.Refer(readPos->offsetBegin, readPos);
            (*readPos).accessPos = chunkPos;
            (*readPos).accessLength = offset - (*readPos).offsetBegin;
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

size_t CacheMediaChunkBufferImpl::GetBufferSize(int64_t offset)
{
    std::lock_guard lock(mutex_);
    auto readPos = GetOffsetFragmentCache(readPos_, offset, LeftBoundedRightOpenComp);
    size_t bufferSize = 0;
    while (readPos != fragmentCacheBuffer_.end()) {
        int64_t nextOffsetBegin = readPos->offsetBegin + readPos->dataLength;
        bufferSize = static_cast<size_t>(nextOffsetBegin - offset);
        readPos++;
        if (readPos == fragmentCacheBuffer_.end() || nextOffsetBegin != readPos->offsetBegin) {
            break;
        }
    }
    return bufferSize;
}

void CacheMediaChunkBufferImpl::HandleFragmentPos(FragmentIterator& fragmentIter)
{
    int64_t nextOffsetBegin = fragmentIter->offsetBegin + fragmentIter->dataLength;
    ++fragmentIter;
    while (fragmentIter != fragmentCacheBuffer_.end()) {
        if (nextOffsetBegin != fragmentIter->offsetBegin) {
            break;
        }
        nextOffsetBegin = fragmentIter->offsetBegin + fragmentIter->dataLength;
        ++fragmentIter;
    }
}

size_t CacheMediaChunkBufferImpl::GetNextBufferOffset(int64_t offset)
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
        return (size_t)(fragmentIter->offsetBegin);
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

inline size_t WriteOneChunkData(CacheChunk& chunkInfo, uint8_t* src, int64_t offset, size_t writeSize)
{
    auto copyBegin = offset - chunkInfo.offset;
    if (copyBegin < 0 || copyBegin > chunkInfo.chunkSize) {
        return 0;
    }
    size_t writePerOne = static_cast<size_t>(chunkInfo.chunkSize - static_cast<size_t>(copyBegin));
    writePerOne = std::min(writePerOne, writeSize);
    errno_t res = memcpy_s(chunkInfo.data + copyBegin, writePerOne, src, writePerOne);
    FALSE_RETURN_V_MSG_E(res == EOK, 0, "memcpy_s data err");
    chunkInfo.dataLength = static_cast<uint32_t>(static_cast<size_t>(copyBegin) + writePerOne);
    return writePerOne;
}

size_t CacheMediaChunkBufferImpl::WriteChunk(FragmentCacheBuffer& fragmentCacheBuffer, ChunkIterator& chunkPos,
                                             void* ptr, int64_t offset, size_t writeSize)
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
    while (writedTmp < writeSize) {
        auto chunkOffset = offset + static_cast<int64_t>(writedTmp);
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
    fragment.offsetBegin += dataLength;
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

inline CacheChunk* PopFreeCacheChunk(CacheChunkList& freeChunks, int64_t offset)
{
    if (freeChunks.empty()) {
        return nullptr;
    }
    auto tmp = freeChunks.front();
    freeChunks.pop_front();
    InitChunkInfo(*tmp, offset);
    return tmp;
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
    }
    freeChunks_.splice(freeChunks_.end(), fragmentIterator->chunks);
    EraseFragmentCache(fragmentIterator);
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

CacheChunk* CacheMediaChunkBufferImpl::GetFreeCacheChunk(int64_t offset, bool checkAllowFailContinue)
{
    if (writePos_ == fragmentCacheBuffer_.end()) {
        return nullptr;
    }
    auto currWritePos = GetOffsetFragmentCache(writePos_, offset, BoundedIntervalComp);
    size_t allowChunkNum = 0;
    if (currWritePos != fragmentCacheBuffer_.end()) {
        allowChunkNum = CalcAllowMaxChunkNum(currWritePos->totalReadSize, currWritePos->offsetBegin);
        DeleteHasReadFragmentCacheBuffer(currWritePos, allowChunkNum);
        if (currWritePos->chunks.size() >= allowChunkNum && !checkAllowFailContinue) {
            return nullptr;
        }
    } else {
        MEDIA_LOG_D("curr write is new fragment.");
    }
    if (!freeChunks_.empty()) {
        return PopFreeCacheChunk(freeChunks_, offset);
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
    while (fragmentCacheBuffer_.size() > CACHE_FRAGMENT_MIN_NUM_DEFAULT) {
        auto result = CheckThresholdFragmentCacheBuffer(currWritePos);
        if (!freeChunks_.empty()) {
            return PopFreeCacheChunk(freeChunks_, offset);
        }
        if (!result) {
            break;
        }
    }
    MEDIA_LOG_D("clear other fragment unread chunk.");
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

ChunkIterator CacheMediaChunkBufferImpl::SplitFragmentCacheBuffer(FragmentIterator& currFragmentIter,
    int64_t offset, ChunkIterator chunkPos)
{
    ResetReadSizeAlloc();

    auto& chunkInfo = *chunkPos;
    CacheChunk* splitHead = nullptr;
    if (offset != chunkInfo->offset) {
        splitHead = freeChunks_.empty() ? GetFreeCacheChunk(offset, true) : PopFreeCacheChunk(freeChunks_, offset);
        if (splitHead == nullptr) {
            return currFragmentIter->chunks.end();
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
        auto diff = offset - chunkInfo->offset;
        if (chunkInfo->dataLength >= diff) {
            splitHead->dataLength = chunkInfo->dataLength - static_cast<uint32_t>(diff);
            chunkInfo->dataLength = static_cast<uint32_t>(diff);
            memcpy_s(splitHead->data, splitHead->dataLength, chunkInfo->data + diff, splitHead->dataLength);
        } else {
            splitHead->dataLength = 0; // It can't happen. us_asan can check.
        }
    }
    newFragmentPos->offsetBegin = offset;
    newFragmentPos->dataLength = currFragmentIter->dataLength - (offset - currFragmentIter->offsetBegin);
    newFragmentPos->accessLength = 0;
    uint64_t newReadSizeInit = static_cast<uint64_t>(1 + initReadSizeFactor_ * static_cast<double>(totalReadSize_));
    if (currFragmentIter->totalReadSize > newReadSizeInit) {
        newReadSizeInit = currFragmentIter->totalReadSize;
    }
    newFragmentPos->totalReadSize = newReadSizeInit;
    totalReadSize_ += newReadSizeInit;

    newFragmentPos->readTime = Clock::now();
    newFragmentPos->accessPos = newFragmentPos->chunks.begin();

    currFragmentIter->dataLength = offset - currFragmentIter->offsetBegin;
    currFragmentIter = newFragmentPos;

    if (fragmentCacheBuffer_.size() > CACHE_FRAGMENT_MAX_NUM_DEFAULT) {
        CheckThresholdFragmentCacheBuffer(currFragmentIter);
    }
    lruCache_.Refer(newFragmentPos->offsetBegin, newFragmentPos);
    return newFragmentPos->accessPos;
}

ChunkIterator CacheMediaChunkBufferImpl::AddFragmentCacheBuffer(int64_t offset)
{
    if (fragmentCacheBuffer_.size() >= CACHE_FRAGMENT_MAX_NUM_DEFAULT) {
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

bool CacheMediaChunkBufferImpl::DumpAndCheckInner()
{
    DumpInner(0);
    return CheckInner();
}

void CacheMediaChunkBufferImpl::CheckFragment(const FragmentCacheBuffer& fragment, bool& checkSuccess)
{
    if (fragment.accessPos != fragment.chunks.end()) {
        auto& accessChunk = *fragment.accessPos;
        auto accessLength = accessChunk->offset - fragment.offsetBegin;
        if (fragment.accessLength < accessLength ||
            fragment.accessLength > (accessLength + static_cast<int64_t>(accessChunk->dataLength))) {
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
            if (chunkPrev->offset + static_cast<int64_t>(chunkPrev->dataLength) != chunkNext->offset) {
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

size_t CacheMediaChunkBuffer::Read(void* ptr, int64_t offset, size_t readSize)
{
    return impl_->Read(ptr, offset, readSize);
}

size_t CacheMediaChunkBuffer::Write(void* ptr, int64_t offset, size_t writeSize)
{
    return impl_->Write(ptr, offset, writeSize);
}

bool CacheMediaChunkBuffer::Seek(int64_t offset)
{
    return impl_->Seek(offset);
}

size_t CacheMediaChunkBuffer::GetBufferSize(int64_t offset)
{
    return impl_->GetBufferSize(offset);
}

size_t CacheMediaChunkBuffer::GetNextBufferOffset(int64_t offset)
{
    return impl_->GetNextBufferOffset(offset);
}

void CacheMediaChunkBuffer::Clear()
{
    impl_->Clear();
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