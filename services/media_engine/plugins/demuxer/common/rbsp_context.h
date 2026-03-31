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
class RbspContext {
public:
    RbspContext(const uint8_t *buf, int32_t size);
    ~RbspContext() = default;
    bool RbspCheckSize(uint32_t size) const;
    void RbspSkipBits(uint32_t size);
    uint32_t RbspGetLeftBytesNum() const;
    uint32_t GetBytesIndex() const;

    template <typename T = uint8_t, typename U = uint32_t>
    T RbspGetBits(uint32_t size, bool isSkip = true)
    {
        if (!RbspCheckSize(size)) {
            RbspSkipBits(size);
            return 0;
        }

        U reverseData = 0;
        uint32_t byteCount = (bitIndex_ + size) % 0x08 == 0 ? (bitIndex_ + size) / 0x08 : (bitIndex_ + size) / 0x08 + 1;
        for (uint32_t i = 0; i < byteCount; ++i) {
            if (byteIndex_ + i >= size_) {
                return 0;
            }
            reverseData = (reverseData << 0x08) | (*(buf_ + byteIndex_ + i) & 0xFF);
        }
        reverseData = (sizeof(U) >= byteCount) ? reverseData << (0x08 * (sizeof(U) - byteCount)) : reverseData;
        T data = static_cast<T>((reverseData << bitIndex_) >> (0x08 * sizeof(U) - size));

        if (isSkip) {
            RbspSkipBits(size);
        }
        return data;
    }

private:
    uint8_t RbspGetBit() const;

    const uint8_t* buf_;
    uint32_t size_;
    uint32_t byteIndex_;
    uint32_t bitIndex_;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // RBSP_CONTEXT_H