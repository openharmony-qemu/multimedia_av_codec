/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_PLUGINS_MEMORY_H
#define HISTREAMER_PLUGINS_MEMORY_H

#include <cstdint>
#include <memory>
#include "buffer/avallocator.h"

namespace OHOS {
namespace Media {
namespace Plugins {
struct Allocator {
    explicit Allocator(MemoryType type = MemoryType::SHARED_MEMORY) : memoryType(type) {}
    virtual ~Allocator() = default;
    /**
     * @brief Allocates a buffer using the specified size
     * .
     * @param size  Allocation parameters.
     * @return      Pointer of the allocated buffer.
     */
    virtual void* Alloc(size_t size) = 0;

    /**
     * @brief Frees a buffer.
     * Buffer handles returned by Alloc() must be freed with this function when no longer needed.
     *
     * @param ptr   Pointer of the allocated buffer.
     */
    virtual void Free(void* ptr) = 0; // NOLINT: intentionally using void* here

    MemoryType GetMemoryType()
    {
        return memoryType;
    }

private:
    MemoryType memoryType;
};
class Memory {
public:
    virtual ~Memory() = default;
    virtual size_t GetCapacity() = 0;
    virtual size_t GetSize() = 0;
    virtual const uint8_t* GetReadOnlyData(size_t position = 0) = 0;
    virtual uint8_t* GetWritableAddr(size_t estimatedWriteSize, size_t position = 0) = 0;
    virtual void UpdateDataSize(size_t realWriteSize, size_t position = 0) = 0;
    virtual size_t Write(const uint8_t* in, size_t writeSize, size_t position = INVALID_POSITION) = 0;
    virtual size_t Read(uint8_t* out, size_t readSize, size_t position = INVALID_POSITION) = 0;
    virtual void Reset() = 0;
    virtual MemoryType GetMemoryType() = 0;
    virtual uint8_t* GetRealAddr() const = 0;
protected:
    explicit Memory(size_t capacity, std::shared_ptr<Allocator> allocator = nullptr,
           size_t align = 1, MemoryType type = MemoryType::VIRTUAL_MEMORY, bool allocMem = true) {}
    MemoryType memoryType;
    size_t capacity;
#if (defined(__GNUC__) || defined(__clang__)) && (!defined(WIN32))
    __attribute__((unused))
#endif
    size_t alignment;
    size_t offset {0};
    size_t size;
    std::shared_ptr<Allocator> allocator;
private:
    Memory(size_t capacity, std::shared_ptr<uint8_t> bufData,
           size_t align = 1, MemoryType type = MemoryType::VIRTUAL_MEMORY) {}
    std::shared_ptr<uint8_t> addr;
    friend class Buffer;
};
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PLUGIN_MEMORY_H
