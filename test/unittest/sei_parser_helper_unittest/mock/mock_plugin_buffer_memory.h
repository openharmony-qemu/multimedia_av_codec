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

#ifndef MOCK_PLUGIN_BUFFER_MEMORY_H
#define MOCK_PLUGIN_BUFFER_MEMORY_H
 
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "plugin_memory.h"
#include "plugin_buffer.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class MockMemory;
class MockBuffer : public Buffer {
public:
    MockBuffer() = default;
    ~MockBuffer() override = default;
    MOCK_METHOD(std::shared_ptr<Memory>, WrapMemory, (uint8_t* data, size_t capacity, size_t size), (override));
    MOCK_METHOD(std::shared_ptr<Memory>, WrapMemoryPtr,
        (std::shared_ptr<uint8_t> data, size_t capacity, size_t size), (override));
    MOCK_METHOD(std::shared_ptr<Memory>, AllocMemory,
        (std::shared_ptr<Allocator> allocator, size_t capacity, size_t align), (override));
    MOCK_METHOD(uint32_t, GetMemoryCount, (), (override));
    MOCK_METHOD(std::shared_ptr<Memory>, GetMemory, (), (override));
    MOCK_METHOD(void, Reset, (), (override));
    MOCK_METHOD(bool, IsEmpty, (), (override));
#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
    MOCK_METHOD(std::shared_ptr<Memory>, WrapSurfaceMemory, (sptr<SurfaceBuffer> surfaceBuffer), (override));
#endif
};

class MockMemory : public Memory {
public:
    explicit MockMemory(size_t capacity, std::shared_ptr<Allocator> allocator = nullptr,
               size_t align = 1, MemoryType type = MemoryType::VIRTUAL_MEMORY,  bool allocMem = true)
        : Memory(capacity, allocator, align, type, allocMem) {}

    MOCK_METHOD(size_t, GetCapacity, (), (override));
    MOCK_METHOD(size_t, GetSize, (), (override));
    MOCK_METHOD(const uint8_t*, GetReadOnlyData, (size_t position), (override));
    MOCK_METHOD(uint8_t*, GetWritableAddr, (size_t estimatedWriteSize, size_t position), (override));
    MOCK_METHOD(void, UpdateDataSize, (size_t realWriteSize, size_t position), (override));
    MOCK_METHOD(size_t, Write, (const uint8_t* in, size_t writeSize, size_t position), (override));
    MOCK_METHOD(size_t, Read, (uint8_t* out, size_t readSize, size_t position), (override));
    MOCK_METHOD(void, Reset, (), (override));
    MOCK_METHOD(MemoryType, GetMemoryType, (), (override));
    MOCK_METHOD(uint8_t*, GetRealAddr, (), (const, override));
};
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // MOCK_PLUGIN_BUFFER_MEMORY_H