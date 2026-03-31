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
#include <set>
#include "rbsp_context.h"

namespace OHOS {
namespace Media {
namespace Plugins {
RbspContext::RbspContext(const uint8_t *buf, int32_t size)
{
    buf_ = buf;
    size_ = static_cast<uint32_t>(size);
    byteIndex_ = 0;
    bitIndex_ = 0;
}

bool RbspContext::RbspCheckSize(uint32_t size) const
{
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
    if (byteIndex_ >= size_) {
        return;
    }
    byteIndex_ = byteIndex_ + (bitIndex_ + size) / 0x08;
    bitIndex_ = (bitIndex_ + size) % 0x08;
}

uint8_t RbspContext::RbspGetBit() const
{
    if (buf_ == nullptr || RbspGetLeftBytesNum() == 0) {
        return 0;
    }
    uint8_t data = 0;
    if (static_cast<uint32_t>(size_) > byteIndex_) {
        data = *(buf_ + byteIndex_);
    }
    data = (data >> (0x07 - bitIndex_)) & 0x01;
    return data;
}

uint32_t RbspContext::RbspGetLeftBytesNum() const
{
    return byteIndex_ >= size_ ? 0 : size_ - byteIndex_;
}

uint32_t RbspContext::GetBytesIndex() const
{
    return byteIndex_;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS