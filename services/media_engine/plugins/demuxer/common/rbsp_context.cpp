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
#include <algorithm>
#include <limits>
#include <set>
#include "rbsp_context.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
constexpr uint32_t RBSP_MAX_UE_LEADING_ZERO_BITS = sizeof(uint32_t) * RBSP_BYTE_BITS;
constexpr uint32_t RBSP_EXP_GOLOMB_SIGNED_DIVISOR = 2;
constexpr uint32_t RBSP_EMULATION_PREVENTION_ZERO_COUNT = 2;
constexpr uint8_t RBSP_EMULATION_PREVENTION_BYTE = 0x03;
}

RbspContext::RbspContext(const uint8_t *buf, int32_t size) : RbspContext(buf, size, false) {}

RbspContext::RbspContext(const uint8_t *buf, int32_t size, bool skipEmulationPreventionByte)
{
    buf_ = buf;
    size_ = static_cast<uint32_t>(size);
    byteIndex_ = 0;
    bitIndex_ = 0;
    zeroCount_ = 0;
    skipEmulationPreventionByte_ = skipEmulationPreventionByte;
}

bool RbspContext::RbspCheckSize(uint32_t size) const
{
    if (size == 0) {
        return true;
    }
    if (byteIndex_ >= size_) {
        return false;
    }

    uint32_t byteIndex = byteIndex_ + (bitIndex_ + size) / 0x08;
    uint32_t leftBitIndex = (bitIndex_ + size) % 0x08;
    if (byteIndex > size_ || (byteIndex == size_ && leftBitIndex > 0)) {
        return false;
    }
    return true;
}

void RbspContext::RbspSkipBits(uint32_t size)
{
    if (size == 0) {
        return;
    }
    if (byteIndex_ >= size_) {
        return;
    }
    byteIndex_ = byteIndex_ + (bitIndex_ + size) / 0x08;
    bitIndex_ = (bitIndex_ + size) % 0x08;
}

uint32_t RbspContext::GetBytesIndex() const
{
    return byteIndex_;
}

bool RbspContext::RbspReadBit(uint32_t& bit)
{
    if (skipEmulationPreventionByte_) {
        return RbspReadEbspBit(bit);
    }
    return RbspReadBits(1, bit);
}

bool RbspContext::RbspReadBits(uint32_t size, uint32_t& value)
{
    value = 0;
    if (size == 0) {
        return true;
    }
    if (size > sizeof(value) * RBSP_BYTE_BITS) {
        return false;
    }
    if (skipEmulationPreventionByte_) {
        for (uint32_t i = 0; i < size; ++i) {
            uint32_t bit = 0;
            if (!RbspReadEbspBit(bit)) {
                return false;
            }
            value = (value << 1) | bit;
        }
        return true;
    }
    if (!RbspCheckSize(size)) {
        return false;
    }
    value = RbspGetBits<uint32_t, uint64_t>(size);
    return true;
}

bool RbspContext::RbspReadUE(uint32_t& value)
{
    uint32_t leadingZeroBits = 0;
    uint32_t bit = 0;
    while (RbspReadBit(bit) && bit == 0) {
        ++leadingZeroBits;
        if (leadingZeroBits > RBSP_MAX_UE_LEADING_ZERO_BITS) {
            return false;
        }
    }
    if (bit == 0) {
        return false;
    }
    uint32_t suffix = 0;
    if (leadingZeroBits > 0 && !RbspReadBits(leadingZeroBits, suffix)) {
        return false;
    }
    uint64_t codeNum = (static_cast<uint64_t>(1) << leadingZeroBits) - 1 + suffix;
    if (codeNum > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    value = static_cast<uint32_t>(codeNum);
    return true;
}

bool RbspContext::RbspReadSE(int32_t& value)
{
    uint32_t codeNum = 0;
    if (!RbspReadUE(codeNum)) {
        return false;
    }
    uint64_t magnitude = ((static_cast<uint64_t>(codeNum) + 1) / RBSP_EXP_GOLOMB_SIGNED_DIVISOR);
    if ((codeNum & 0x01) != 0) {
        if (magnitude > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
            return false;
        }
        value = static_cast<int32_t>(magnitude);
        return true;
    }
    value = -static_cast<int32_t>(codeNum / RBSP_EXP_GOLOMB_SIGNED_DIVISOR);
    return true;
}

bool RbspContext::RbspReadEbspBit(uint32_t& bit)
{
    bit = 0;
    SkipEmulationPreventionByte();
    if (buf_ == nullptr || byteIndex_ >= size_) {
        return false;
    }
    bit = (*(buf_ + byteIndex_) >> (0x07 - bitIndex_)) & 0x01;
    ++bitIndex_;
    if (bitIndex_ < RBSP_BYTE_BITS) {
        return true;
    }
    bitIndex_ = 0;
    if (*(buf_ + byteIndex_) == 0x00) {
        ++zeroCount_;
    } else {
        zeroCount_ = 0;
    }
    ++byteIndex_;
    return true;
}

void RbspContext::SkipEmulationPreventionByte()
{
    if (bitIndex_ != 0 || byteIndex_ >= size_) {
        return;
    }
    if (zeroCount_ >= RBSP_EMULATION_PREVENTION_ZERO_COUNT &&
        *(buf_ + byteIndex_) == RBSP_EMULATION_PREVENTION_BYTE) {
        zeroCount_ = 0;
        ++byteIndex_;
    }
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS
