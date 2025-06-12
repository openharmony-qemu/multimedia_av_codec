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

#ifndef HISTREAMER_PLUGINS_COMMON_BUFFER_H
#define HISTREAMER_PLUGINS_COMMON_BUFFER_H

#include "gmock/gmock.h"
#include <memory>
#include <map>
#include <vector>

#include "mock/plugin_memory.h"
#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
#include "refbase.h"
#include "surface/surface.h"
#endif

namespace OHOS {
namespace Media {
namespace Plugins {

// Align value template
template <typename T>
using MakeUnsigned = typename std::make_unsigned<T>::type;

template <typename T, typename U>
constexpr T AlignUp(T num, U alignment)
{
    return (alignment > 0) ? (static_cast<uint64_t>((num + static_cast<MakeUnsigned<T>>(alignment) - 1)) &
        static_cast<uint64_t>((~(static_cast<MakeUnsigned<T>>(alignment) - 1)))) :
        num;
}

/**
* @brief Buffer base class.
* Contains the data storage of the buffer (buffer description information).
*
* @since 1.0
* @version 1.0
*/
class Buffer {
public:
    Buffer() = default;
    ~Buffer() = default;
    static std::shared_ptr<Buffer> CreateDefaultBuffer(size_t capacity, std::shared_ptr<Allocator> allocator = nullptr,
                                                       size_t align = 1)
    {
        return nullptr;
    }
    MOCK_METHOD(std::shared_ptr<Memory>, WrapMemory, (uint8_t* data, size_t capacity, size_t size), ());
    MOCK_METHOD(std::shared_ptr<Memory>, WrapMemoryPtr,
        (std::shared_ptr<uint8_t> data, size_t capacity, size_t size), ());
    MOCK_METHOD(std::shared_ptr<Memory>, AllocMemory,
        (std::shared_ptr<Allocator> allocator, size_t capacity, size_t align), ());
    MOCK_METHOD(uint32_t, GetMemoryCount, (), ());
    std::shared_ptr<Memory> GetMemory(uint32_t index = 0)
    {
        std::shared_ptr<uint8_t> bufData = nullptr;
        return std::make_shared<Memory>(index, bufData);
    }
    MOCK_METHOD(void, Reset, (), ());
    MOCK_METHOD(bool, IsEmpty, (), ());
#if !defined(OHOS_LITE) && defined(VIDEO_SUPPORT)
    MOCK_METHOD(std::shared_ptr<Memory>, WrapSurfaceMemory, (sptr<SurfaceBuffer> surfaceBuffer), ());
#endif
    int32_t streamID;
    uint32_t trackID;
    int64_t pts;
    int64_t dts;
    int64_t duration;
    uint64_t flag;
    std::vector<std::shared_ptr<Memory>> data {};
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PLUGINS_COMMON_BUFFER_H
