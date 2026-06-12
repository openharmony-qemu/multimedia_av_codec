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

#ifndef RBSP_CONTEXT_H
#define RBSP_CONTEXT_H

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr uint32_t RBSP_BYTE_BITS = 0x08;

class RbspContext {
public:
    RbspContext(const uint8_t *buf, int32_t size);
    RbspContext(const uint8_t *buf, int32_t size, bool skipEmulationPreventionByte);
    ~RbspContext() = default;
    bool RbspCheckSize(uint32_t size) const;
    void RbspSkipBits(uint32_t size);
    uint32_t GetBytesIndex() const;
    bool RbspReadBit(uint32_t& bit);
    bool RbspReadBits(uint32_t size, uint32_t& value);
    bool RbspReadUE(uint32_t& value);
    bool RbspReadSE(int32_t& value);

    template <typename T = uint8_t, typename U = uint32_t>
    T RbspGetBits(uint32_t size, bool isSkip = true)
    {
        constexpr uint32_t valueBits = sizeof(T) * RBSP_BYTE_BITS;
        constexpr uint32_t workBits = sizeof(U) * RBSP_BYTE_BITS;
        constexpr uint32_t accumulatorBits = sizeof(uint64_t) * RBSP_BYTE_BITS;
        if (size == 0 || size > valueBits || size > workBits || size > accumulatorBits) {
            return 0;
        }

        if (!RbspCheckSize(size)) {
            RbspSkipBits(size);
            return 0;
        }

        uint64_t data = 0;
        for (uint32_t i = 0; i < size; ++i) {
            uint64_t totalBitOffset = static_cast<uint64_t>(bitIndex_) + i;
            uint64_t currentByteIndex = static_cast<uint64_t>(byteIndex_) + totalBitOffset / RBSP_BYTE_BITS;
            if (currentByteIndex >= size_) {
                return 0;
            }
            uint32_t currentBitIndex = static_cast<uint32_t>(totalBitOffset % RBSP_BYTE_BITS);
            uint32_t shiftBits = (RBSP_BYTE_BITS - 1) - currentBitIndex;
            uint64_t currentByte = static_cast<uint64_t>(*(buf_ + static_cast<uint32_t>(currentByteIndex)));
            uint8_t bit = static_cast<uint8_t>((currentByte >> shiftBits) & 0x01);
            data = (data << 1) | bit;
        }

        if (isSkip) {
            RbspSkipBits(size);
        }
        return static_cast<T>(data);
    }

private:
    bool RbspReadEbspBit(uint32_t& bit);
    void SkipEmulationPreventionByte();

    const uint8_t* buf_;
    uint32_t size_;
    uint32_t byteIndex_;
    uint32_t bitIndex_;
    uint32_t zeroCount_;
    bool skipEmulationPreventionByte_;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // RBSP_CONTEXT_H
