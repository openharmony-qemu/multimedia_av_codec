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

#ifndef HISTREAMER_CACHED_MEDIA_BUFFER_H
#define HISTREAMER_CACHED_MEDIA_BUFFER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <list>
#include <chrono>

#include "common/log.h"
#include "lru_cache.h"

namespace OHOS {
namespace Media {
constexpr uint32_t CHUNK_SIZE = 16 * 1024;
constexpr uint64_t MAX_CACHE_BUFFER_SIZE = 20 * 1024 * 1024;

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using namespace std::chrono;

struct CacheChunk {
    uint32_t chunkSize;
    uint32_t dataLength;
    int64_t offset;
    uint8_t data[];
};

using CacheChunkList = std::list<CacheChunk*>;
using ChunkIterator = CacheChunkList::iterator;

struct FragmentCacheBuffer {
    int64_t offsetBegin;
    int64_t dataLength;
    int64_t accessLength;
    uint64_t totalReadSize;
    TimePoint readTime;
    CacheChunkList chunks;
    ChunkIterator accessPos;

    explicit FragmentCacheBuffer(int64_t offset = 0) : offsetBegin(offset), dataLength(0),
        accessLength(0), totalReadSize(0), readTime(Clock::now())
    {
        accessPos = chunks.end();
    }

    ~FragmentCacheBuffer()
    {
        chunks.clear();
    }
};

using FragmentCacheBufferList = std::list<FragmentCacheBuffer>;
using FragmentIterator = FragmentCacheBufferList::iterator;

class CacheMediaChunkBufferImpl {
public:
    CacheMediaChunkBufferImpl();
    ~CacheMediaChunkBufferImpl();

    CacheMediaChunkBufferImpl(const CacheMediaChunkBufferImpl&) = delete;
    CacheMediaChunkBufferImpl(CacheMediaChunkBufferImpl&&) = delete;
    const CacheMediaChunkBufferImpl& operator=(const CacheMediaChunkBufferImpl&) = delete;
    CacheMediaChunkBufferImpl& operator=(CacheMediaChunkBufferImpl&&) = delete;

    bool Init(uint64_t totalBuffSize, uint32_t chunkSize);
    size_t Read(void* ptr, int64_t offset, size_t readSize);
    size_t Write(void* ptr, int64_t inOffset, size_t inWriteSize);
    bool Seek(int64_t offset);
    size_t GetBufferSize(int64_t offset);
    size_t GetNextBufferOffset(int64_t offset);
    void Dump(uint64_t param);
    bool Check();
    void Clear();

protected:
    CacheChunk* GetFreeCacheChunk(int64_t offset, bool checkAllowFailContinue = false);
    FragmentIterator EraseFragmentCache(const FragmentIterator& iter);
    FragmentIterator GetOffsetFragmentCache(FragmentIterator& fragmentPos, int64_t offset);
    ChunkIterator GetOffsetChunkCache(CacheChunkList& fragmentCacheBuffer, int64_t offset);
    static void DumpInner(uint64_t param);
    bool CheckInner();
    void CheckFragment(const FragmentCacheBuffer& fragment, bool& checkSuccess);
    bool DumpAndCheckInner();
    static void UpdateAccessPos(FragmentIterator& fragmentPos, ChunkIterator& chunkPos, int64_t offsetChunk);
    bool WriteInPlace(FragmentIterator& fragmentPos, uint8_t* ptr, int64_t inOffset,
                      size_t inWriteSize, size_t& outWriteSize);
    bool WriteMergerPre(int64_t offset, size_t writeSize, FragmentIterator& nextFragmentPos);
    void WriteMergerPost(FragmentIterator& nextFragmentPos);
    size_t ReadInner(void* ptr, int64_t offset, size_t readSize);

    template<typename Pred>
    FragmentIterator GetOffsetFragmentCache(FragmentIterator& fragmentPos, int64_t offset, Pred pred)
    {
        if (fragmentPos != fragmentCacheBuffer_.end()) {
            if (pred(offset, fragmentPos->offsetBegin, fragmentPos->offsetBegin + fragmentPos->dataLength)) {
                return fragmentPos;
            }
        }

        auto fragmentCachePos = std::find_if(fragmentCacheBuffer_.begin(), fragmentCacheBuffer_.end(),
            [offset, pred](const auto& fragment) {
                if (pred(offset, fragment.offsetBegin, fragment.offsetBegin + fragment.dataLength)) {
                    return true;
                }
                return false;
        });
        return fragmentCachePos;
    }

    template<typename Pred>
    static ChunkIterator GetOffsetChunkCache(CacheChunkList& chunkCaches, int64_t offset, Pred pred)
    {
        auto chunkCachePos = std::find_if(chunkCaches.begin(), chunkCaches.end(),
            [offset, pred](const auto& fragment) {
                if (pred(offset, fragment->offset, fragment->offset + fragment->dataLength)) {
                    return true;
                }
                return false;
        });
        return chunkCachePos;
    }

    size_t WriteChunk(FragmentCacheBuffer& fragmentCacheBuffer, ChunkIterator& chunkPos,
                      void* ptr, int64_t offset, size_t writeSize);
    bool CheckThresholdFragmentCacheBuffer(FragmentIterator& currWritePos);
    ChunkIterator AddFragmentCacheBuffer(int64_t offset);
    ChunkIterator SplitFragmentCacheBuffer(FragmentIterator& currFragmentIter, int64_t offset, ChunkIterator chunkPos);

    void DeleteHasReadFragmentCacheBuffer(FragmentIterator& fragmentIter, size_t allowChunkNum);
    void DeleteUnreadFragmentCacheBuffer(FragmentIterator& fragmentIter, size_t allowChunkNum);
    size_t CalcAllowMaxChunkNum(uint64_t fragmentReadSize, int64_t offset)
    {
        size_t allowNum = static_cast<size_t>((static_cast<double>(fragmentReadSize) /
            static_cast<double>(totalReadSize_)) * chunkMaxNum_);
        return allowNum;
    }
    void ResetReadSizeAlloc();
    CacheChunk* UpdateFragmentCacheForDelHead(FragmentIterator& fragmentIter);
    void HandleFragmentPos(FragmentIterator& fragmentIter);

private:
    std::mutex mutex_;
    FragmentIterator readPos_;
    FragmentIterator writePos_;
    uint64_t totalBuffSize_ {0};
    uint64_t totalReadSize_ {0};
    uint32_t chunkMaxNum_ {0};
    uint32_t chunkSize_ {0};
    double initReadSizeFactor_ {0.0};
    uint8_t* bufferAddr_ {nullptr};
    FragmentCacheBufferList fragmentCacheBuffer_;
    CacheChunkList freeChunks_;
    size_t fragmentMaxNum_;
    LruCache<int64_t, FragmentIterator> lruCache_;
};

class CacheMediaBuffer {
public:
    CacheMediaBuffer() = default;
    virtual ~CacheMediaBuffer() = default;

    virtual bool Init(uint64_t totalBuffSize, uint32_t chunkSize) = 0;
    virtual size_t Read(void* ptr, int64_t offset, size_t readSize) = 0;
    virtual size_t Write(void* ptr, int64_t offset, size_t writeSize) = 0;
    virtual bool Seek(int64_t offset) = 0;
    virtual size_t GetBufferSize(int64_t offset) = 0;
    virtual size_t GetNextBufferOffset(int64_t offset) = 0;
    virtual void Clear() = 0;
    virtual void SetReadBlocking(bool isReadBlockingAllowed) = 0;
    virtual void Dump(uint64_t param) = 0;
};

class CacheMediaChunkBufferImpl;
class CacheMediaChunkBuffer : public CacheMediaBuffer {
public:
    CacheMediaChunkBuffer();
    ~CacheMediaChunkBuffer() override;
    CacheMediaChunkBuffer(const CacheMediaChunkBuffer&) = delete;
    CacheMediaChunkBuffer(CacheMediaChunkBuffer&&) = delete;
    const CacheMediaChunkBuffer& operator=(const CacheMediaChunkBuffer&) = delete;
    CacheMediaChunkBuffer& operator=(CacheMediaChunkBuffer&&) = delete;

    bool Init(uint64_t totalBuffSize, uint32_t chunkSize) override;
    size_t Read(void* ptr, int64_t offset, size_t readSize) override;
    size_t Write(void* ptr, int64_t offset, size_t writeSize) override;
    bool Seek(int64_t offset) override;
    size_t GetBufferSize(int64_t offset) override;
    size_t GetNextBufferOffset(int64_t offset) override;
    void Clear() override;
    void SetReadBlocking(bool isReadBlockingAllowed) override;
    void Dump(uint64_t param) override;
    bool Check();
private:
    std::unique_ptr<CacheMediaChunkBufferImpl> impl_;
};

}
}
#endif