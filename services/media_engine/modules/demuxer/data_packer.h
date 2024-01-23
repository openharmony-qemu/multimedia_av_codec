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

#ifndef DATA_PACKER_H
#define DATA_PACKER_H

#include <atomic>
#include <deque>
#include <vector>
#include "plugin/plugin_buffer.h"
#include "osal/task/mutex.h"
#include "osal/task/condition_variable.h"

namespace OHOS {
namespace Media {
class DataPacker {
public:
    DataPacker();

    ~DataPacker();

    DataPacker(const DataPacker& other) = delete;

    DataPacker& operator=(const DataPacker& other) = delete;

    void PushData(std::shared_ptr<Plugins::Buffer>& bufferPtr, uint64_t offset);

    bool IsDataAvailable(uint64_t offset, uint32_t size);

    bool PeekRange(uint64_t offset, uint32_t size, std::shared_ptr<Plugins::Buffer>& bufferPtr);

    bool GetRange(uint64_t offset, uint32_t size, std::shared_ptr<Plugins::Buffer>& bufferPtr);

    bool GetRange(uint32_t size, std::shared_ptr<Plugins::Buffer>& bufferPtr,
        uint64_t preRemoveOffset, bool isPreRemove); // For live play

    void PreRemove(uint64_t preRemoveOffset, bool isPreRemove);

    bool GetOrWaitDataAvailable(uint64_t offset, uint32_t size);

    void Flush();

    void SetEos();

    void IsSupportPreDownload(bool isSupport);

    bool IsEmpty();

    void Start();

    void Stop();

    // Record the position that GerRange copy start or end.
    struct Position {
        int32_t index; // Buffer index, -1 means this Position is invalid
        uint32_t bufferOffset; // Offset in the buffer
        uint64_t mediaOffset;  // Offset in the media file

        Position(int32_t index, uint32_t bufferOffset, uint64_t mediaOffset) noexcept
        {
            this->index = index;
            this->bufferOffset = bufferOffset;
            this->mediaOffset = mediaOffset;
        }

        bool operator < (const Position& other) const
        {
            if (index < 0 || other.index < 0) { // Position invalid
                return false;
            }
            if (index != other.index) {
                return index < other.index;
            }
            return bufferOffset < other.bufferOffset; // use bufferOffset, maybe live play mediaOffset is invalid
        }

        std::string ToString() const
        {
            return "Position (index " + std::to_string(index) + ", bufferOffset " + std::to_string(bufferOffset) +
                ", mediaOffset " + std::to_string(mediaOffset);
        }
    };

private:
    void RemoveBufferContent(std::shared_ptr<Plugins::Buffer> &buffer, size_t removeSize);

    bool PeekRangeInternal(uint64_t offset, uint32_t size, std::shared_ptr<Plugins::Buffer>& bufferPtr, bool isGet);

    bool IsDataAvailableInternal(uint64_t offset, uint32_t size);

    void FlushInternal();

    bool FindFirstBufferToCopy(uint64_t offset, int32_t &startIndex, uint64_t &prevOffset);

    size_t CopyFirstBuffer(size_t size, int32_t index, uint8_t *dstPtr, std::shared_ptr<Plugins::Buffer>& dstBufferPtr,
        int32_t bufferOffset);

    int32_t CopyFromSuccessiveBuffer(uint64_t prevOffset, uint64_t offsetEnd, int32_t startIndex, uint8_t *dstPtr,
        uint32_t &needCopySize);

    void RemoveOldData(const Position& position);

    bool RemoveTo(const Position& position);

    bool UpdateWhenFrontDataRemoved(size_t removeSize);

    std::string ToString() const;

    Mutex mutex_;
    std::deque<std::shared_ptr<Plugins::Buffer>> que_;
    std::atomic<uint32_t> size_;
    uint64_t mediaOffset_; // The media file offset of the first byte in data packer
    uint64_t pts_;
    uint64_t dts_;
    bool isEos_ {false};
    bool isSupportPreDownload_ {false};

    // The position in prev GetRange, current GetRange
    Position prevGet_ ;
    Position currentGet_ ;

    ConditionVariable cvFull_;
    ConditionVariable cvEmpty_;
    ConditionVariable cvAllowRead_;
    size_t capacity_;
    std::atomic<bool> stopped_ {false};
};
} // namespace Media
} // namespace OHOS
#endif // DATA_PACKER_H
